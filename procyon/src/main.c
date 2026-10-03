#include "procyon.h"
#include "util.h"

int uninstall(void) {
    procyon_restore_backup();
    remove_at_path("/mnt1/private/var/root/procyon");
    remove_at_path("/mnt1/private/etc/racoon/stage2");

    sync_volume("/mnt1/private/var");
    sync_volume("/mnt1/");
    usleep(250000);
    return 0;
}

int update(void) {
    print_log(false, "[*] updating untether, this will take a few minutes...\n");
    if (procyon_init() != 0) {
        print_log(true, "[-] failed to initialize, untether will NOT be updated\n");
        return -1;
    }
    
    remove_at_path("/mnt1/private/etc/racoon/racoon.conf");
    if (procyon->use_dhcpd) {
        remove_at_path("/mnt1/private/etc/dhcpd.conf");
    }

    remove_at_path("/mnt1/private/var/root/procyon/stage1");
    remove_at_path("/mnt1/private/etc/racoon/stage2");
    remove_at_path("/mnt1/private/var/root/procyon/stage3.bin");

    mkdir("/mnt1/private/var/root/procyon/stage1", 0777);
    chown("/mnt1/private/var/root/procyon/stage1", 0, 0);
    int status = -1;

    if (gen_stage1() != 0) {
        print_log(true, "[-] failed to create stage1\n");
        goto done;
    }

    if (gen_stage2() != 0) {
        print_log(true, "[-] failed to create stage2\n");
        goto done;
    }

    if (gen_stage3() != 0) {
        print_log(true, "[-] failed to create stage3\n");
        goto done;
    }

    print_log(false, "[*] cleaning up...\n");
    sync_volume("/mnt1/private/var");
    sync_volume("/mnt1/");
    usleep(250000);
    status = 0;

done:
    if (status == 0) return 0;
    print_log(true, "[-] failed to update untether\n");
    return -1;
}


int install(void) {
    if (access("/mnt1/private/var/root/procyon", F_OK) == 0) return update();
    print_log(false, "[*] installing untether, this will take a few minutes...\n");
    
    if (procyon_init() != 0) {
        print_log(true, "[-] failed to initialize, untether will NOT be installed\n");
        return -1;
    }

    mkdir("/mnt1/private/var/root/procyon", 0777);
    chown("/mnt1/private/var/root/procyon", 0, 0);
    mkdir("/mnt1/private/var/root/procyon/stage1", 0777);
    chown("/mnt1/private/var/root/procyon/stage1", 0, 0);
    int status = -1;

    if (procyon_create_backup() != 0) {
        print_log(true, "[-] failed to create backup\n");
        goto done;
    }

    size_t disabled_size = 0;
    void *disabled_data = load_embedded_file("__disabled", &disabled_size);
    if (disabled_data == NULL) {
        print_log(true, "[-] failed to load __disabled section\n");
        goto done;
    }

    move_file("/mnt1/private/var/db/com.apple.xpc.launchd/disabled.plist", "/mnt1/private/var/db/com.apple.xpc.launchd/disabled_orig.plist", true);
    chmod("/mnt1/private/var/db/com.apple.xpc.launchd/disabled_orig.plist", 0400);

    int fd = open("/mnt1/private/var/db/com.apple.xpc.launchd/disabled.plist", O_RDWR|O_CREAT, 0644);
    if (fd < 0) {
        print_log(true, "[-] failed to create /var/db/com.apple.xpc.launchd/disabled.plist\n");
        goto done;
    }

    write(fd, disabled_data, disabled_size);
    fcntl(fd, F_FULLFSYNC);
    close(fd);
    
    chown("/mnt1/private/var/db/com.apple.xpc.launchd/disabled.plist", 0, 0);
    move_file("/mnt1/usr/libexec/wifiFirmwareLoaderLegacy", "/mnt1/usr/libexec/wifiFirmwareLoaderLegacy_orig", true);
    remove_at_path("/mnt1/private/etc/racoon/racoon.conf");

    if (procyon->use_dhcpd) {
        symlink("/mnt1/usr/libexec/dhcpd", "/mnt1/usr/libexec/wifiFirmwareLoaderLegacy");
        remove_at_path("/mnt1/private/etc/dhcpd.conf");
    } else {
        symlink("/mnt1/usr/sbin/racoon", "/mnt1/usr/libexec/wifiFirmwareLoaderLegacy");
    }

    if (gen_stage1() != 0) {
        print_log(true, "[-] failed to create stage1\n");
        goto done;
    }

    if (gen_stage2() != 0) {
        print_log(true, "[-] failed to create stage2\n");
        goto done;
    }

    if (gen_stage3() != 0) {
        print_log(true, "[-] failed to create stage3\n");
        goto done;
    }

    print_log(false, "[*] cleaning up...\n");
    sync_volume("/mnt1/private/var");
    sync_volume("/mnt1/");
    usleep(250000);
    status = 0;

done:
    if (status == 0) return 0;
    print_log(true, "[-] failed to install untether, undoing all changes...\n");
    uninstall();
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) return -1;
    if (strcmp(argv[1], "install") == 0) return install();
    if (strcmp(argv[1], "uninstall") == 0) return uninstall();
    if (strcmp(argv[1], "update") == 0) return update();
    return 0;
}
