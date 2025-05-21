#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

#define MODULE_DIR "/sys/module/"
#define CUSTOM_MOD_DIR "../modules/"
#define MAX_MODNAME_LEN 64

void clear_screen() {
    printf("\033[H\033[J");
}

int is_module_loaded(const char *modname) {
    char path[256];
    snprintf(path, sizeof(path), "%s%s", MODULE_DIR, modname);
    return access(path, F_OK) == 0;
}

void get_custom_modules(char modules[][MAX_MODNAME_LEN], int *count) {
    DIR *dir;
    struct dirent *ent;
    *count = 0;
    
    // First check which modules are available in custom directory
    dir = opendir(CUSTOM_MOD_DIR);
    if (!dir) {
        perror("Failed to open custom modules directory");
        return;
    }

    while ((ent = readdir(dir)) && *count < 100) {
        if (ent->d_name[0] == '.') continue;
        
        char *ext = strrchr(ent->d_name, '.');
        if (ext && strcmp(ext, ".ko") == 0) {
            strncpy(modules[*count], ent->d_name, ext - ent->d_name);
            modules[*count][ext - ent->d_name] = '\0';
            (*count)++;
        }
    }
    closedir(dir);
}

void print_menu(const char custom_mods[][MAX_MODNAME_LEN], int mod_count) {
    clear_screen();
    printf("=== Custom Kernel Module Manager ===\n");
    printf("Detected modules in %s:\n", CUSTOM_MOD_DIR);
    
    for (int i = 0; i < mod_count; i++) {
        printf("%2d. %-20s [%s]\n", 
               i+1, 
               custom_mods[i],
               is_module_loaded(custom_mods[i]) ? "LOADED" : "NOT LOADED");
    }
    
    printf("\nOptions:\n");
    printf("  %2d. Load module\n", mod_count+1);
    printf("  %2d. Unload module\n", mod_count+2);
    printf("  %2d. Refresh list\n", mod_count+3);
    printf("  %2d. View dmesg\n", mod_count+4);
    printf("  %2d. Exit\n", mod_count+5);
    printf("Select option: ");
}

void load_module(const char *modname) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo insmod %s%s.ko", CUSTOM_MOD_DIR, modname);
    
    printf("Loading module %s...\n", modname);
    if (system(cmd))
    {
        printf("Failed to load module. Check dmesg for details.\n");
    } else {
        printf("Module loaded successfully!\n");
    }
    sleep(1);
}

void unload_module(const char *modname) {
    if (!is_module_loaded(modname)) {
        printf("Module %s is not loaded!\n", modname);
        sleep(1);
        return;
    }
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo rmmod %s", modname);
    
    printf("Unloading module %s...\n", modname);
    if (system(cmd)) {
        printf("Failed to unload module. Is it in use?\n");
    } else {
        printf("Module unloaded successfully!\n");
    }
    sleep(1);
}

void view_dmesg() {
    clear_screen();
    printf("=== Last kernel messages ===\n");
    system("sudo dmesg | tail -20");
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}



int main() {
    char custom_mods[100][MAX_MODNAME_LEN];
    int mod_count = 0;
    int choice;
    
    // Replace $(uname -r) with actual kernel version
    char resolved_path[256];

    while (1) {
        get_custom_modules(custom_mods, &mod_count);
        print_menu(custom_mods, mod_count);
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        if (choice > 0 && choice <= mod_count) {
            // Selected one of the modules
            if (is_module_loaded(custom_mods[choice-1])) {
                char confirm;
                printf("Module %s is loaded. Unload it? (y/n): ", custom_mods[choice-1]);
                scanf(" %c", &confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    unload_module(custom_mods[choice-1]);
                }
            } else {
                load_module(custom_mods[choice-1]);
            }
        } 
        else if (choice == mod_count + 1) {
            // Load module
            printf("Enter module name (without .ko): ");
            char modname[MAX_MODNAME_LEN];
            scanf("%63s", modname);
            load_module(modname);
        }
        else if (choice == mod_count + 2) {
            // Unload module
            printf("Enter module name: ");
            char modname[MAX_MODNAME_LEN];
            scanf("%63s", modname);
            unload_module(modname);
        }
        else if (choice == mod_count + 3) {
            // Refresh - will happen automatically
            continue;
        }
        else if (choice == mod_count + 4) {
            view_dmesg();
        }
        else if (choice == mod_count + 5) {
            break; // Exit
        }
    }
    
    printf("Exiting module manager.\n");
    return 0;
}

// Helper function to get kernel version
const char* get_kernel_version() {
    static char version[64];
    FILE *f = fopen("/proc/version", "r");
    if (f) {
        if (fscanf(f, "Linux version %63s", version) != 1) {
            strcpy(version, "unknown");
        }
        fclose(f);
    }
    return version;
}
