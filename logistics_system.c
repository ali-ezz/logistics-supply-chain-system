#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Define base paths
char CURRENT_DIR[PATH_MAX];
char LOGISTICS_BASE_PATH[PATH_MAX];
char ADMIN_BASE_PATH[PATH_MAX];
char WAREHOUSE_BASE_PATH[PATH_MAX];
char CUSTOMER_BASE_PATH[PATH_MAX];

// Alias structures
typedef struct Alias {
    char name[256];
    char command[256];
} Alias;

Alias admin_aliases[10];
int admin_alias_count = 0;
Alias warehouse_aliases[10];
int warehouse_alias_count = 0;

// User context structure
typedef struct UserContext {
    const char **base_paths;
    int base_paths_count;
    const char *user_type;
    Alias *aliases;
    int *alias_count;
} UserContext;

// Function prototypes
void initialize_paths();
int sanitize_filename(const char *filename, char *sanitized, size_t size);
int is_valid_path(const char **base_paths, int base_paths_count, const char *path);
void list_files(UserContext *user_ctx);
void change_permissions(UserContext *user_ctx);
void create_directory(UserContext *user_ctx);
void delete_directory(UserContext *user_ctx);
void create_file(UserContext *user_ctx);
void delete_file(UserContext *user_ctx);
void create_symbolic_link(UserContext *user_ctx);
void copy_file(UserContext *user_ctx);
void move_file(UserContext *user_ctx);
void append_to_file(UserContext *user_ctx);
void view_file_content(UserContext *user_ctx);
void find_file(UserContext *user_ctx);
void search_content(UserContext *user_ctx);
void set_alias(UserContext *user_ctx);
void use_alias(UserContext *user_ctx);
void main_menu(UserContext *user_ctx);
void select_user_type();
char *get_input(const char *prompt, char *buffer, size_t size);
int login_user();
const char* select_base_path_with_other(UserContext *user_ctx, const char *prompt);

// Base paths arrays
const char *admin_base_paths[3];
const char *warehouse_base_paths[2];
const char *customer_base_paths[1];

int main() {
    initialize_paths();
    select_user_type();
    return 0;
}

// Initialize directory paths and create them if they don't exist
void initialize_paths() {
    int ret;

    // Get the current working directory
    if (getcwd(CURRENT_DIR, sizeof(CURRENT_DIR)) == NULL) {
        perror("Error getting current directory");
        exit(EXIT_FAILURE);
    }

    // Initialize LOGISTICS_BASE_PATH
    ret = snprintf(LOGISTICS_BASE_PATH, PATH_MAX, "%s/logistics", CURRENT_DIR);
    if (ret < 0 || (size_t)ret >= PATH_MAX) {
        fprintf(stderr, "Error initializing LOGISTICS_BASE_PATH.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize ADMIN_BASE_PATH
    ret = snprintf(ADMIN_BASE_PATH, PATH_MAX, "%s/admin", LOGISTICS_BASE_PATH);
    if (ret < 0 || (size_t)ret >= PATH_MAX) {
        fprintf(stderr, "Error initializing ADMIN_BASE_PATH.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize WAREHOUSE_BASE_PATH
    ret = snprintf(WAREHOUSE_BASE_PATH, PATH_MAX, "%s/warehouse", LOGISTICS_BASE_PATH);
    if (ret < 0 || (size_t)ret >= PATH_MAX) {
        fprintf(stderr, "Error initializing WAREHOUSE_BASE_PATH.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize CUSTOMER_BASE_PATH
    ret = snprintf(CUSTOMER_BASE_PATH, PATH_MAX, "%s/customers", LOGISTICS_BASE_PATH);
    if (ret < 0 || (size_t)ret >= PATH_MAX) {
        fprintf(stderr, "Error initializing CUSTOMER_BASE_PATH.\n");
        exit(EXIT_FAILURE);
    }

    // Assign base paths to arrays
    admin_base_paths[0] = ADMIN_BASE_PATH;
    admin_base_paths[1] = WAREHOUSE_BASE_PATH;
    admin_base_paths[2] = CUSTOMER_BASE_PATH;

    warehouse_base_paths[0] = WAREHOUSE_BASE_PATH;
    warehouse_base_paths[1] = CUSTOMER_BASE_PATH;

    customer_base_paths[0] = CUSTOMER_BASE_PATH;

    // Create directories if they do not exist using system commands
    char command[PATH_MAX + 20];
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", LOGISTICS_BASE_PATH);
    system(command);
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", ADMIN_BASE_PATH);
    system(command);
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", WAREHOUSE_BASE_PATH);
    system(command);
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", CUSTOMER_BASE_PATH);
    system(command);
}

// Sanitize filename to prevent directory traversal
int sanitize_filename(const char *filename, char *sanitized, size_t size) {
    if (filename == NULL || filename[0] == '\0') return 0;

    // Check for absolute path
    if (filename[0] == '/') return 0;

    // Check for invalid characters
    if (strstr(filename, "..") != NULL || strchr(filename, '/') != NULL || strchr(filename, '\\') != NULL)
        return 0;

    // Ensure the filename is within allowed length
    if (strlen(filename) >= size)
        return 0;

    // Copy the filename into the sanitized buffer
    strncpy(sanitized, filename, size);
    sanitized[size - 1] = '\0';

    return 1;  // Success
}

// Check if path is within any of the base paths allowed for the user
int is_valid_path(const char **base_paths, int base_paths_count, const char *path) {
    char real_target[PATH_MAX];

    // Attempt to resolve target path
    if (realpath(path, real_target) == NULL) {
        // If the target path does not exist yet, resolve its parent directory
        char path_copy[PATH_MAX];
        strncpy(path_copy, path, PATH_MAX);
        path_copy[PATH_MAX - 1] = '\0';

        char *last_slash = strrchr(path_copy, '/');
        if (last_slash != NULL && last_slash != path_copy) {
            *last_slash = '\0';  // Remove file/directory name to get parent directory
        } else {
            // Cannot resolve parent directory
            return 0;
        }

        if (realpath(path_copy, real_target) == NULL) {
            perror("Error resolving target path in is_valid_path");
            return 0;
        }
    }

    // Check if the real target path starts with any of the allowed base paths
    for (int i = 0; i < base_paths_count; i++) {
        char real_base[PATH_MAX];
        if (realpath(base_paths[i], real_base) == NULL) {
            perror("Error resolving base path in is_valid_path");
            continue;
        }

        size_t base_len = strlen(real_base);
        if (strncmp(real_base, real_target, base_len) == 0 &&
            (real_target[base_len] == '/' || real_target[base_len] == '\0')) {
            return 1;  // Valid path
        }
    }

    return 0;  // Path is not within allowed base paths
}

// Get input from user
char *get_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline
        return buffer;
    }
    return NULL;
}

// Function to select base path with 'Other' option
const char* select_base_path_with_other(UserContext *user_ctx, const char *prompt) {
    printf("%s\n", prompt);
    for (int i = 0; i < user_ctx->base_paths_count; i++) {
        printf("%d. %s\n", i + 1, user_ctx->base_paths[i]);
    }
    printf("%d. Other (specify subdirectory under allowed directories)\n", user_ctx->base_paths_count + 1);

    char dir_choice_str[10];
    if (get_input("Enter your choice: ", dir_choice_str, sizeof(dir_choice_str)) == NULL) {
        printf("Error reading input.\n");
        return NULL;
    }
    int dir_choice = atoi(dir_choice_str) - 1;
    if (dir_choice < 0 || dir_choice > user_ctx->base_paths_count) {
        printf("Invalid choice.\n");
        return NULL;
    }
    if (dir_choice == user_ctx->base_paths_count) {
        // 'Other' option selected
        printf("Select the base directory under which the subdirectory is located:\n");
        for (int i = 0; i < user_ctx->base_paths_count; i++) {
            printf("%d. %s\n", i + 1, user_ctx->base_paths[i]);
        }
        if (get_input("Enter your choice: ", dir_choice_str, sizeof(dir_choice_str)) == NULL) {
            printf("Error reading input.\n");
            return NULL;
        }
        int base_dir_choice = atoi(dir_choice_str) - 1;
        if (base_dir_choice < 0 || base_dir_choice >= user_ctx->base_paths_count) {
            printf("Invalid choice.\n");
            return NULL;
        }
        const char *selected_base_path = user_ctx->base_paths[base_dir_choice];
        char subdir_name[256];
        if (get_input("Enter the subdirectory path under selected base directory: ", subdir_name, sizeof(subdir_name)) == NULL) {
            printf("Error reading input.\n");
            return NULL;
        }
        // Sanitize subdirectory name
        char sanitized_subdir[256];
        if (!sanitize_filename(subdir_name, sanitized_subdir, sizeof(sanitized_subdir))) {
            printf("Invalid subdirectory name.\n");
            return NULL;
        }
        // Now construct the base path
        static char temp_path[PATH_MAX];
        if (snprintf(temp_path, sizeof(temp_path), "%s/%s", selected_base_path, sanitized_subdir) >= (int)sizeof(temp_path)) {
            printf("Path is too long.\n");
            return NULL;
        }
        // Check that it is a valid path under allowed paths
        if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, temp_path)) {
            printf("Invalid or forbidden path.\n");
            return NULL;
        }
        // Check if the directory exists
        struct stat sb;
        if (stat(temp_path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            return temp_path;
        } else {
            printf("Directory does not exist.\n");
            return NULL;
        }
    } else {
        return user_ctx->base_paths[dir_choice];
    }
}

// Function to list files in allowed directories
void list_files(UserContext *user_ctx) {
    printf("Listing files in allowed directories:\n");
    int total_files = 0;
    for (int i = 0; i < user_ctx->base_paths_count; i++) {
        const char *base_path = user_ctx->base_paths[i];
        printf("\nDirectory: %s\n", base_path);

        // Construct the find command
        char command[PATH_MAX + 100];
        snprintf(command, sizeof(command), "find \"%s\" -type f", base_path);

        // Open a pipe to read the output of the command
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            printf("Failed to execute command.\n");
            continue;
        }

        char path_buffer[PATH_MAX];
        int dir_file_count = 0;
        while (fgets(path_buffer, sizeof(path_buffer), fp) != NULL) {
            printf("%s", path_buffer);  // Print the file path
            dir_file_count++;
        }
        pclose(fp);

        printf("Number of files in %s: %d\n", base_path, dir_file_count);
        total_files += dir_file_count;
    }
    printf("\nTotal number of files: %d\n", total_files);
}

// Function to change file permissions
void change_permissions(UserContext *user_ctx) {
    char file_name[256];
    if (get_input("Enter file name to change permissions: ", file_name, sizeof(file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(file_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid file name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory of the file:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }

    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if file exists
    struct stat sb;
    if (stat(full_path, &sb) != 0) {
        printf("File does not exist.\n");
        return;
    }

    char perm_str[10];
    if (get_input("Enter new permissions (e.g., 755): ", perm_str, sizeof(perm_str)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    // Use chmod function
    mode_t mode = strtol(perm_str, NULL, 8);
    if (chmod(full_path, mode) == 0) {
        printf("Permissions changed for %s\n", full_path);
    } else {
        perror("Error changing permissions");
    }
}

// Function to create directory
void create_directory(UserContext *user_ctx) {
    char dir_name[256];
    if (get_input("Enter directory name to create: ", dir_name, sizeof(dir_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(dir_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid directory name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory to create the new directory in:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }

    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if directory exists
    struct stat sb;
    if (stat(full_path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("Directory already exists.\n");
        return;
    }

    // Create directory
    if (mkdir(full_path, 0777) == 0) {
        printf("Directory created: %s\n", full_path);
    } else {
        perror("Error creating directory");
    }
}

// Function to delete directory
void delete_directory(UserContext *user_ctx) {
    char dir_name[256];
    if (get_input("Enter directory name to delete: ", dir_name, sizeof(dir_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(dir_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid directory name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory where the directory to delete is located:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }

    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if the directory exists
    struct stat sb;
    if (stat(full_path, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        printf("Directory does not exist.\n");
        return;
    }

    // Delete directory using Linux command
    char command[PATH_MAX + 20];
    snprintf(command, sizeof(command), "rm -rf \"%s\"", full_path);
    int result = system(command);
    if (result == 0) {
        printf("Directory deleted: %s\n", full_path);
    } else {
        printf("Error deleting directory\n");
    }
}

// Function to create file
void create_file(UserContext *user_ctx) {
    char file_name[256];
    if (get_input("Enter file name to create: ", file_name, sizeof(file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(file_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid file name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory to create the new file in:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }

    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if file exists
    struct stat sb;
    if (stat(full_path, &sb) == 0 && S_ISREG(sb.st_mode)) {
        printf("File already exists.\n");
        return;
    }

    // Create file using touch command
    char command[PATH_MAX + 20];
    snprintf(command, sizeof(command), "touch \"%s\"", full_path);
    int result = system(command);
    if (result == 0) {
        printf("File created: %s\n", full_path);
    } else {
        printf("Error creating file\n");
    }
}

// Function to delete file
void delete_file(UserContext *user_ctx) {
    char file_name[256];
    if (get_input("Enter file name to delete: ", file_name, sizeof(file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(file_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid file name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory where the file is located:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }

    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if file exists
    struct stat sb;
    if (stat(full_path, &sb) != 0 || !S_ISREG(sb.st_mode)) {
        printf("File does not exist.\n");
        return;
    }

    // Delete file using rm command
    char command[PATH_MAX + 20];
    snprintf(command, sizeof(command), "rm \"%s\"", full_path);
    int result = system(command);
    if (result == 0) {
        printf("File deleted: %s\n", full_path);
    } else {
        printf("Error deleting file\n");
    }
}

// Function to create symbolic link
void create_symbolic_link(UserContext *user_ctx) {
    char target[256], link_name[256];
    if (get_input("Enter target file for symbolic link: ", target, sizeof(target)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    if (get_input("Enter symbolic link name: ", link_name, sizeof(link_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_target[256];
    char sanitized_link_name[256];
    if (!sanitize_filename(target, sanitized_target, sizeof(sanitized_target)) ||
        !sanitize_filename(link_name, sanitized_link_name, sizeof(sanitized_link_name))) {
        printf("Invalid file or link name.\n");
        return;
    }

    // Base path selection for target
    const char *target_base_path = select_base_path_with_other(user_ctx, "Select the directory where the target file is located:");
    if (target_base_path == NULL) return;

    // Base path selection for link
    const char *link_base_path = select_base_path_with_other(user_ctx, "Select the directory where the symbolic link will be created:");
    if (link_base_path == NULL) return;

    char full_target_path[PATH_MAX], full_link_path[PATH_MAX];
    if (snprintf(full_target_path, sizeof(full_target_path), "%s/%s", target_base_path, sanitized_target) >= (int)sizeof(full_target_path) ||
        snprintf(full_link_path, sizeof(full_link_path), "%s/%s", link_base_path, sanitized_link_name) >= (int)sizeof(full_link_path)) {
        printf("Path is too long.\n");
        return;
    }

    // Validate paths
    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_target_path) ||
        !is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_link_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if target file exists
    struct stat sb;
    if (stat(full_target_path, &sb) != 0) {
        printf("Target file does not exist.\n");
        return;
    }

    // Create symbolic link using ln -s command
    char command[PATH_MAX * 2 + 20];
    snprintf(command, sizeof(command), "ln -s \"%s\" \"%s\"", full_target_path, full_link_path);
    int result = system(command);
    if (result == 0) {
        printf("Symbolic link created: %s\n", full_link_path);
    } else {
        printf("Error creating symbolic link\n");
    }
}

// Function to copy file
void copy_file(UserContext *user_ctx) {
    char source[256], destination[256];
    if (get_input("Enter source file to copy: ", source, sizeof(source)) == NULL ||
        get_input("Enter destination file name: ", destination, sizeof(destination)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_source[256];
    char sanitized_destination[256];
    if (!sanitize_filename(source, sanitized_source, sizeof(sanitized_source)) ||
        !sanitize_filename(destination, sanitized_destination, sizeof(sanitized_destination))) {
        printf("Invalid source or destination file name.\n");
        return;
    }

    // Base path selection for source
    const char *source_base_path = select_base_path_with_other(user_ctx, "Select the directory where the source file is located:");
    if (source_base_path == NULL) return;

    // Base path selection for destination
    const char *dest_base_path = select_base_path_with_other(user_ctx, "Select the directory where the destination file will be created:");
    if (dest_base_path == NULL) return;

    char full_source_path[PATH_MAX], full_destination_path[PATH_MAX];
    if (snprintf(full_source_path, sizeof(full_source_path), "%s/%s", source_base_path, sanitized_source) >= (int)sizeof(full_source_path) ||
        snprintf(full_destination_path, sizeof(full_destination_path), "%s/%s", dest_base_path, sanitized_destination) >= (int)sizeof(full_destination_path)) {
        printf("Path is too long.\n");
        return;
    }

    // Validate paths
    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_source_path) ||
        !is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_destination_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if source file exists
    struct stat sb;
    if (stat(full_source_path, &sb) != 0) {
        printf("Source file does not exist.\n");
        return;
    }

    // Copy file using cp command
    char command[PATH_MAX * 2 + 20];
    snprintf(command, sizeof(command), "cp \"%s\" \"%s\"", full_source_path, full_destination_path);
    int result = system(command);
    if (result == 0) {
        printf("File copied from %s to %s\n", full_source_path, full_destination_path);
    } else {
        printf("Error copying file\n");
    }
}

// Function to move file
void move_file(UserContext *user_ctx) {
    char source_file_name[256];
    if (get_input("Enter source file to move: ", source_file_name, sizeof(source_file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_source[256];
    if (!sanitize_filename(source_file_name, sanitized_source, sizeof(sanitized_source))) {
        printf("Invalid source file name.\n");
        return;
    }

    // Base path selection for source
    const char *source_base_path = select_base_path_with_other(user_ctx, "Select the directory where the source file is located:");
    if (source_base_path == NULL) return;

    // Base path selection for destination
    const char *dest_base_path = select_base_path_with_other(user_ctx, "Select the directory where the file will be moved to:");
    if (dest_base_path == NULL) return;

    char full_source_path[PATH_MAX], full_destination_path[PATH_MAX];
    if (snprintf(full_source_path, sizeof(full_source_path), "%s/%s", source_base_path, sanitized_source) >= (int)sizeof(full_source_path) ||
        snprintf(full_destination_path, sizeof(full_destination_path), "%s/%s", dest_base_path, sanitized_source) >= (int)sizeof(full_destination_path)) {
        printf("Path is too long.\n");
        return;
    }

    // Validate paths
    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_source_path) ||
        !is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_destination_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if source file exists
    struct stat sb;
    if (stat(full_source_path, &sb) != 0) {
        printf("Source file does not exist.\n");
        return;
    }

    // Move file using mv command
    char command[PATH_MAX * 2 + 20];
    snprintf(command, sizeof(command), "mv \"%s\" \"%s\"", full_source_path, full_destination_path);
    int result = system(command);
    if (result == 0) {
        printf("File moved from %s to %s\n", full_source_path, full_destination_path);
    } else {
        printf("Error moving file\n");
    }
}

// Function to append to file
void append_to_file(UserContext *user_ctx) {
    char file_name[256];
    if (get_input("Enter file name to append text: ", file_name, sizeof(file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(file_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid file name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory where the file is located or will be created:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }
    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    char text[1024];
    if (get_input("Enter text to append: ", text, sizeof(text)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    // Escape special characters in text
    char escaped_text[2048];
    char *src = text;
    char *dst = escaped_text;
    size_t escaped_text_size = sizeof(escaped_text);
    size_t remaining_size = escaped_text_size - 1;  // Reserve space for null terminator

    while (*src && remaining_size > 0) {
        if (*src == '"' || *src == '\\' || *src == '$' || *src == '`') {
            if (remaining_size < 2) {  // Need space for backslash and character
                printf("Error: Escaped text is too long.\n");
                return;
            }
            *dst++ = '\\';
            *dst++ = *src++;
            remaining_size -= 2;
        } else {
            *dst++ = *src++;
            remaining_size--;
        }
    }
    *dst = '\0';

    // Calculate the required command buffer size
    size_t command_size = strlen(escaped_text) + strlen(full_path) + 50;  // Additional space for command syntax

    if (command_size > PATH_MAX + 4096) {  // Set an upper limit to prevent excessively large allocations
        printf("Error: Command is too long.\n");
        return;
    }

    char *command = malloc(command_size);
    if (command == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    int len = snprintf(command, command_size, "echo \"%s\" >> \"%s\"", escaped_text, full_path);
    if (len < 0 || (size_t)len >= command_size) {
        printf("Error: Command buffer size exceeded.\n");
        free(command);
        return;
    }

    int result = system(command);
    free(command);
    if (result == 0) {
        printf("Text appended to %s\n", full_path);
    } else {
        printf("Error appending to file\n");
    }
}

// Function to view file content
void view_file_content(UserContext *user_ctx) {
    char file_name[256];
    if (get_input("Enter file name to view content: ", file_name, sizeof(file_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    char sanitized_name[256];
    if (!sanitize_filename(file_name, sanitized_name, sizeof(sanitized_name))) {
        printf("Invalid file name.\n");
        return;
    }

    const char *base_path = select_base_path_with_other(user_ctx, "Select the directory where the file is located:");
    if (base_path == NULL) return;

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", base_path, sanitized_name) >= (int)sizeof(full_path)) {
        printf("Path is too long.\n");
        return;
    }
    if (!is_valid_path(user_ctx->base_paths, user_ctx->base_paths_count, full_path)) {
        printf("Invalid path. Operation not allowed.\n");
        return;
    }

    // Check if file exists
    struct stat sb;
    if (stat(full_path, &sb) != 0 || !S_ISREG(sb.st_mode)) {
        printf("File does not exist.\n");
        return;
    }

    char option[10];
    if (get_input("View whole file or (h)ead/(t)ail? (w/h/t): ", option, sizeof(option)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    int num_lines = 0;
    char num_str[10];
    if (option[0] == 'h' || option[0] == 'H' || option[0] == 't' || option[0] == 'T') {
        if (get_input("Enter number of lines to display: ", num_str, sizeof(num_str)) == NULL) {
            printf("Error reading input.\n");
            return;
        }
        num_lines = atoi(num_str);
        if (num_lines <= 0) {
            printf("Invalid number of lines.\n");
            return;
        }
    }

    // Construct the command
    char command[PATH_MAX + 50];
    if (option[0] == 'w' || option[0] == 'W') {
        snprintf(command, sizeof(command), "cat \"%s\"", full_path);
    } else if (option[0] == 'h' || option[0] == 'H') {
        snprintf(command, sizeof(command), "head -n %d \"%s\"", num_lines, full_path);
    } else if (option[0] == 't' || option[0] == 'T') {
        snprintf(command, sizeof(command), "tail -n %d \"%s\"", num_lines, full_path);
    } else {
        printf("Invalid option.\n");
        return;
    }

    // Execute the command
    system(command);
}

// Function to find files with pattern
void find_file(UserContext *user_ctx) {
    char pattern[256];
    if (get_input("Enter file name pattern to search (use '*' for wildcards): ", pattern, sizeof(pattern)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    printf("Searching for files matching %s in allowed directories.\n", pattern);

    for (int i = 0; i < user_ctx->base_paths_count; i++) {
        // Construct the find command
        char command[PATH_MAX + 300];
        snprintf(command, sizeof(command), "find \"%s\" -name \"%s\" -print", user_ctx->base_paths[i], pattern);
        // Execute the command
        system(command);
    }
}

// Function to search content in files
void search_content(UserContext *user_ctx) {
    char keyword[256];
    if (get_input("Enter keyword to search in files: ", keyword, sizeof(keyword)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    printf("Searching for keyword '%s' in files under allowed directories.\n", keyword);

    for (int i = 0; i < user_ctx->base_paths_count; i++) {
        // Construct the grep command
        char command[PATH_MAX + 300];
        snprintf(command, sizeof(command), "grep -r \"%s\" \"%s\"", keyword, user_ctx->base_paths[i]);
        // Execute the command
        system(command);
    }
}

// Function to set alias
void set_alias(UserContext *user_ctx) {
    if (user_ctx->aliases == NULL || user_ctx->alias_count == NULL) {
        printf("Aliases not available for this user.\n");
        return;
    }

    char alias_name[256];
    char command[256];
    if (get_input("Enter alias name: ", alias_name, sizeof(alias_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    if (get_input("Enter command to associate with the alias: ", command, sizeof(command)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    if (*(user_ctx->alias_count) < 10) {
        strcpy(user_ctx->aliases[*(user_ctx->alias_count)].name, alias_name);
        strcpy(user_ctx->aliases[*(user_ctx->alias_count)].command, command);
        (*(user_ctx->alias_count))++;
        printf("Alias '%s' set for command '%s'.\n", alias_name, command);
    } else {
        printf("Alias limit reached.\n");
    }
}

// Function to use alias
void use_alias(UserContext *user_ctx) {
    if (user_ctx->aliases == NULL || user_ctx->alias_count == NULL) {
        printf("Aliases not available for this user.\n");
        return;
    }

    char alias_name[256];
    if (get_input("Enter alias to use: ", alias_name, sizeof(alias_name)) == NULL) {
        printf("Error reading input.\n");
        return;
    }

    char command[256] = {0};
    int found = 0;
    for (int i = 0; i < *(user_ctx->alias_count); i++) {
        if (strcmp(user_ctx->aliases[i].name, alias_name) == 0) {
            strcpy(command, user_ctx->aliases[i].command);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Alias not found.\n");
        return;
    }

    // Map command to function
    if (strcmp(command, "list") == 0) {
        list_files(user_ctx);
    } else if (strcmp(command, "move") == 0) {
        move_file(user_ctx);
    } else if (strcmp(command, "append") == 0) {
        append_to_file(user_ctx);
    } else if (strcmp(command, "view") == 0) {
        view_file_content(user_ctx);
    } else if (strcmp(command, "create_dir") == 0) {
        create_directory(user_ctx);
    } else if (strcmp(command, "delete_dir") == 0) {
        delete_directory(user_ctx);
    } else if (strcmp(command, "create_file") == 0) {
        create_file(user_ctx);
    } else if (strcmp(command, "delete_file") == 0) {
        delete_file(user_ctx);
    } else if (strcmp(command, "copy") == 0) {
        copy_file(user_ctx);
    } else if (strcmp(command, "find") == 0) {
        find_file(user_ctx);
    } else if (strcmp(command, "search") == 0) {
        search_content(user_ctx);
    } else if (strcmp(command, "change_perms") == 0) {
        change_permissions(user_ctx);
    } else {
        printf("Command associated with alias '%s' is not recognized.\n", command);
    }
}

// Login function
int login_user() {
    char username[256];
    char password[256];

    if (get_input("Enter username: ", username, sizeof(username)) == NULL) {
        printf("Error reading input.\n");
        return 0;
    }
    if (get_input("Enter password: ", password, sizeof(password)) == NULL) {
        printf("Error reading input.\n");
        return 0;
    }

    if (strcmp(username, "ali") == 0 && strcmp(password, "1") == 0) {
        printf("Login successful.\n");
        return 1;
    } else {
        printf("Invalid username or password.\n");
        return 0;
    }
}

// Main menu for user
void main_menu(UserContext *user_ctx) {
    while (1) {
        printf("\n%s Menu:\n", user_ctx->user_type);
        if (strcmp(user_ctx->user_type, "admin") == 0) {
            printf("1. List files\n");
            printf("2. Change permissions\n");
            printf("3. Create directory\n");
            printf("4. Delete directory\n");
            printf("5. Create file\n");
            printf("6. Delete file\n");
            printf("7. Create symbolic link\n");
            printf("8. Copy file\n");
            printf("9. Move file\n");
            printf("10. Append to file\n");
            printf("11. View file content\n");
            printf("12. Find file\n");
            printf("13. Search file content\n");
            printf("14. Set alias\n");
            printf("15. Use alias\n");
            printf("16. Logout\n");
        } else if (strcmp(user_ctx->user_type, "warehouse") == 0) {
            printf("1. List files\n");
            printf("2. Move file\n");
            printf("3. View file content\n");
            printf("4. Create directory\n");
            printf("5. Delete directory\n");
            printf("6. Create file\n");
            printf("7. Delete file\n");
            printf("8. Append to file\n");
            printf("9. Set alias\n");
            printf("10. Use alias\n");
            printf("11. Logout\n");
        } else if (strcmp(user_ctx->user_type, "customer") == 0) {
            printf("1. List files\n");
            printf("2. Copy file\n");
            printf("3. Append to file\n");
            printf("4. View file content\n");
            printf("5. Logout\n");
        }

        char choice_str[10];
        if (get_input("Choose an option: ", choice_str, sizeof(choice_str)) == NULL) {
            printf("Error reading input.\n");
            continue;
        }
        int choice = atoi(choice_str);

        if (strcmp(user_ctx->user_type, "admin") == 0) {
            switch (choice) {
                case 1:
                    list_files(user_ctx);
                    break;
                case 2:
                    change_permissions(user_ctx);
                    break;
                case 3:
                    create_directory(user_ctx);
                    break;
                case 4:
                    delete_directory(user_ctx);
                    break;
                case 5:
                    create_file(user_ctx);
                    break;
                case 6:
                    delete_file(user_ctx);
                    break;
                case 7:
                    create_symbolic_link(user_ctx);
                    break;
                case 8:
                    copy_file(user_ctx);
                    break;
                case 9:
                    move_file(user_ctx);
                    break;
                case 10:
                    append_to_file(user_ctx);
                    break;
                case 11:
                    view_file_content(user_ctx);
                    break;
                case 12:
                    find_file(user_ctx);
                    break;
                case 13:
                    search_content(user_ctx);
                    break;
                case 14:
                    set_alias(user_ctx);
                    break;
                case 15:
                    use_alias(user_ctx);
                    break;
                case 16:
                    printf("Logging out.\n");
                    return;
                default:
                    printf("Invalid choice.\n");
                    break;
            }
        } else if (strcmp(user_ctx->user_type, "warehouse") == 0) {
            switch (choice) {
                case 1:
                    list_files(user_ctx);
                    break;
                case 2:
                    move_file(user_ctx);
                    break;
                case 3:
                    view_file_content(user_ctx);
                    break;
                case 4:
                    create_directory(user_ctx);
                    break;
                case 5:
                    delete_directory(user_ctx);
                    break;
                case 6:
                    create_file(user_ctx);
                    break;
                case 7:
                    delete_file(user_ctx);
                    break;
                case 8:
                    append_to_file(user_ctx);
                    break;
                case 9:
                    set_alias(user_ctx);
                    break;
                case 10:
                    use_alias(user_ctx);
                    break;
                case 11:
                    printf("Logging out.\n");
                    return;
                default:
                    printf("Invalid choice.\n");
                    break;
            }
        } else if (strcmp(user_ctx->user_type, "customer") == 0) {
            switch (choice) {
                case 1:
                    list_files(user_ctx);
                    break;
                case 2:
                    copy_file(user_ctx);
                    break;
                case 3:
                    append_to_file(user_ctx);
                    break;
                case 4:
                    view_file_content(user_ctx);
                    break;
                case 5:
                    printf("Logging out.\n");
                    return;
                default:
                    printf("Invalid choice.\n");
                    break;
            }
        } else {
            printf("Invalid user type.\n");
            return;
        }
    }
}

// User type selection
void select_user_type() {
    while (1) {
        printf("\nSelect User Type:\n");
        printf("1. Admin\n");
        printf("2. Warehouse Staff\n");
        printf("3. Customer\n");
        printf("4. Exit\n");
        char choice_str[10];
        if (get_input("Enter your choice: ", choice_str, sizeof(choice_str)) == NULL) {
            printf("Error reading input.\n");
            continue;
        }
        int choice = atoi(choice_str);

        UserContext user_ctx;
        memset(&user_ctx, 0, sizeof(UserContext));

        if (choice == 1) {
            user_ctx.user_type = "admin";
            user_ctx.base_paths = admin_base_paths;
            user_ctx.base_paths_count = 3;
            user_ctx.aliases = admin_aliases;
            user_ctx.alias_count = &admin_alias_count;
        } else if (choice == 2) {
            user_ctx.user_type = "warehouse";
            user_ctx.base_paths = warehouse_base_paths;
            user_ctx.base_paths_count = 2;
            user_ctx.aliases = warehouse_aliases;
            user_ctx.alias_count = &warehouse_alias_count;
        } else if (choice == 3) {
            user_ctx.user_type = "customer";
            user_ctx.base_paths = customer_base_paths;
            user_ctx.base_paths_count = 1;
            user_ctx.aliases = NULL;
            user_ctx.alias_count = NULL;
        } else if (choice == 4) {
            printf("Exiting.\n");
            break;
        } else {
            printf("Invalid choice.\n");
            continue;
        }

        if (login_user()) {
            main_menu(&user_ctx);
        } else {
            printf("Login failed. Returning to user type selection.\n");
        }
    }
}

/*
نظرة عامة
هذا البرنامج هو تطبيق سطر أوامر يحاكي نظام إدارة ملفات مبسط لشركة لوجستية. يسمح لمستخدمين من أدوار مختلفة (المسؤول، موظفي المستودعات، والعملاء) بتنفيذ عمليات ملفات مختلفة داخل أدلة محددة. يتضمن البرنامج ميزات مثل إنشاء وحذف الملفات والأدلة، تغيير الأذونات، نسخ ونقل الملفات، وأكثر. كما يدعم البرنامج استخدام الأسماء المستعارة للأوامر، مما يوفر طريقة لتنفيذ المهام الشائعة بسهولة أكبر.

محتوى الشرح
استيراد المكتبات
تعريف المتغيرات العامة والهياكل
تصريحات الدوال (Function Prototypes)
الدالة الرئيسية (main)
دالة تهيئة المسارات (initialize_paths)
دالة تنقية اسم الملف (sanitize_filename)
دالة التحقق من صلاحية المسار (is_valid_path)
دالة الحصول على الإدخال من المستخدم (get_input)
دالة اختيار المسار الأساسي مع خيار "أخرى" (select_base_path_with_other)
دوال إجراءات المستخدم
عرض الملفات
تغيير الأذونات
إنشاء دليل
حذف دليل
إنشاء ملف
حذف ملف
إنشاء رابط رمزي
نسخ ملف
نقل ملف
إضافة نص إلى ملف
عرض محتوى ملف
البحث عن ملف
البحث في المحتوى
دوال إدارة الأسماء المستعارة
تعيين اسم مستعار
استخدام اسم مستعار
دالة تسجيل الدخول
دالة القائمة الرئيسية
دالة اختيار نوع المستخدم
الشرح التفصيلي
1. استيراد المكتبات
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>


الغرض: تتضمن هذه الأسطر استيراد مكتبات لغة C القياسية التي توفر الدوال والتعريفات اللازمة في جميع أنحاء البرنامج.
توضيح المكتبات:
stdio.h: دوال الإدخال/الإخراج القياسية (مثل printf، scanf، fgets).
stdlib.h: دوال المكتبة القياسية (مثل malloc، free، system).
string.h: دوال التعامل مع السلاسل النصية (مثل strcpy، strcat، strlen).
unistd.h: توفر الوصول إلى واجهات نظام التشغيل POSIX (مثل getcwd، access).
limits.h: تعريف الثوابت لحدود أنواع البيانات (مثل PATH_MAX).
sys/wait.h: تصريحات لدوال wait (على الرغم من عدم استخدامها مباشرة، قد تم تضمينها للكمال).
sys/stat.h: تعريفات لدوال حالة الملفات (مثل stat، chmod، mkdir).
2. تعريف المتغيرات العامة والهياكل
المتغيرات العامة للمسارات
// تعريف المسارات الأساسية
char CURRENT_DIR[PATH_MAX];
char LOGISTICS_BASE_PATH[PATH_MAX];
char ADMIN_BASE_PATH[PATH_MAX];
char WAREHOUSE_BASE_PATH[PATH_MAX];
char CUSTOMER_BASE_PATH[PATH_MAX];


الغرض: هذه المصفوفات من الأحرف تخزن المسارات إلى الأدلة المختلفة المستخدمة في البرنامج.
المتغيرات:
CURRENT_DIR: دليل العمل الحالي حيث يتم تنفيذ البرنامج.
LOGISTICS_BASE_PATH: الدليل الأساسي للتطبيق اللوجستي.
ADMIN_BASE_PATH: الدليل الخاص بملفات المسؤول.
WAREHOUSE_BASE_PATH: الدليل الخاص بملفات المستودع.
CUSTOMER_BASE_PATH: الدليل الخاص بملفات العملاء.
ملاحظة: PATH_MAX هو ثابت يحدد الحد الأقصى لطول المسار على النظام.
هياكل الأسماء المستعارة
// هياكل الأسماء المستعارة
typedef struct Alias {
    char name[256];
    char command[256];
} Alias;

Alias admin_aliases[10];
int admin_alias_count = 0;
Alias warehouse_aliases[10];
int warehouse_alias_count = 0;


الغرض: يسمح للمستخدمين بإنشاء اختصارات مخصصة (أسماء مستعارة) للأوامر.
الهيكل:
Alias: هيكل يحتوي على اسم الأمر والأمر المرتبط به.
المتغيرات:
admin_aliases: مصفوفة من الأسماء المستعارة للمسؤول.
admin_alias_count: عداد لعدد الأسماء المستعارة للمسؤول.
warehouse_aliases: مصفوفة من الأسماء المستعارة لموظفي المستودع.
warehouse_alias_count: عداد لعدد الأسماء المستعارة للمستودع.
ملاحظة: العملاء ليس لديهم أسماء مستعارة في هذا الإعداد.
هيكل سياق المستخدم
// هيكل سياق المستخدم
typedef struct UserContext {
    const char **base_paths;
    int base_paths_count;
    const char *user_type;
    Alias *aliases;
    int *alias_count;
} UserContext;


الغرض: يخزن معلومات حول جلسة المستخدم الحالية والأذونات.
الحقول:
base_paths: مصفوفة من المسارات التي يمكن للمستخدم الوصول إليها.
base_paths_count: عدد المسارات الأساسية.
user_type: نوع المستخدم ("admin"، "warehouse"، أو "customer").
aliases: مؤشر إلى مصفوفة الأسماء المستعارة للمستخدم.
alias_count: مؤشر إلى عدد الأسماء المستعارة للمستخدم.
3. تصريحات الدوال (Function Prototypes)
// تصريحات الدوال
void initialize_paths();
int sanitize_filename(const char *filename, char *sanitized, size_t size);
// ... (تصريحات باقي الدوال)


الغرض: يعلن عن جميع الدوال المستخدمة في البرنامج قبل تعريفها، مما يسمح للمترجم بفهم استخدامها في الكود.
ملاحظة: تصريحات الدوال مهمة في لغة C لإعلام المترجم بأسماء الدوال، وأنواع الإرجاع، وأنواع المعاملات.
4. الدالة الرئيسية (main)
int main() {
    initialize_paths();
    select_user_type();
    return 0;
}


الغرض: نقطة الدخول للبرنامج.
العمليات:
يستدعي initialize_paths لإعداد هيكل الدليل.
يستدعي select_user_type لبدء عملية تفاعل المستخدم.
يعيد القيمة 0، مما يشير إلى تنفيذ ناجح.
5. دالة تهيئة المسارات (initialize_paths)
void initialize_paths() {
    int ret;
    // الحصول على دليل العمل الحالي
    // تهيئة LOGISTICS_BASE_PATH
    // تهيئة ADMIN_BASE_PATH
    // تهيئة WAREHOUSE_BASE_PATH
    // تهيئة CUSTOMER_BASE_PATH
    // تعيين المسارات الأساسية إلى المصفوفات
    // إنشاء الأدلة إذا لم تكن موجودة
}


الغرض: يعد الأدلة اللازمة للبرنامج ويهيئ المتغيرات العامة للمسارات.
الخطوات:
الحصول على دليل العمل الحالي: يستخدم getcwd لتخزين الدليل الحالي في CURRENT_DIR.
تهيئة المسارات الأساسية: يستخدم snprintf لإنشاء المسارات لأدلة اللوجستيات، المسؤول، المستودع، والعملاء بناءً على CURRENT_DIR.
تعيين المسارات الأساسية إلى المصفوفات:
admin_base_paths: يحتوي على المسارات التي يمكن للمسؤول الوصول إليها.
warehouse_base_paths: يحتوي على المسارات لموظفي المستودع.
customer_base_paths: يحتوي على المسارات للعملاء.
إنشاء الأدلة: يستخدم استدعاءات system مع mkdir -p لإنشاء الأدلة إذا لم تكن موجودة.
ملاحظة: على الرغم من استخدام system هنا، يفضل استخدام دالة mkdir من sys/stat.h لأمان وتحكم أفضل.
6. دالة تنقية اسم الملف (sanitize_filename)
int sanitize_filename(const char *filename, char *sanitized, size_t size) {
    // تحقق ونسخ اسم الملف إلى المخزن المنقح
    // يعيد 1 عند النجاح، 0 عند الفشل
}


الغرض: يتحقق وينقي أسماء الملفات التي يقدمها المستخدم لمنع مشاكل الأمان مثل هجمات اجتياز الدليل.
التحقق:
سلسلة فارغة أو NULL: يتأكد من أن اسم الملف ليس فارغًا أو NULL.
مسار مطلق: يمنع أسماء الملفات التي تبدأ بـ / لمنع المسارات المطلقة.
أحرف غير صالحة: يمنع ..، /، و \ لمنع التنقل إلى الأدلة الرئيسية أو تحديد أدلة فرعية.
تحقق الطول: يتأكد من أن اسم الملف لا يتجاوز حجم المخزن المؤقت المقدم.
العملية:
ينسخ اسم الملف المنقى إلى المخزن المؤقت المقدم.
يعيد 1 لاسم ملف صالح، 0 خلاف ذلك.
7. دالة التحقق من صلاحية المسار (is_valid_path)
int is_valid_path(const char **base_paths, int base_paths_count, const char *path) {
    // يتحقق مما إذا كان المسار المقدم ضمن المسارات الأساسية المسموح بها
    // يعيد 1 إذا كان صالحًا، 0 خلاف ذلك
}


الغرض: يتأكد من أن أي عمليات على الملفات أو الأدلة تتم داخل الأدلة المسموح بها للمستخدم.
العملية:
حل المسار الحقيقي: يستخدم realpath لحل الروابط الرمزية والحصول على المسار المطلق.
التحقق من الدليل الأصلي: إذا لم يكن المسار موجودًا بعد، يحل دليل الأصلي.
المقارنة مع المسارات الأساسية: يتحقق مما إذا كان المسار المحلول يبدأ بأي من المسارات الأساسية المسموح بها.
أهمية للأمان: يمنع المستخدمين من الوصول أو تعديل الملفات خارج أدلتهم المخصصة.
8. دالة الحصول على الإدخال من المستخدم (get_input)
char *get_input(const char *prompt, char *buffer, size_t size) {
    // يعرض المطالبة ويقرأ الإدخال في المخزن المؤقت
    // يزيل حرف السطر الجديد
    // يعيد المخزن المؤقت عند النجاح، NULL عند الفشل
}


الغرض: يتعامل مع إدخال المستخدم بأمان وبشكل متسق في جميع أنحاء البرنامج.
ملاحظات:
يستخدم fgets لتجنب تجاوزات المخزن المؤقت.
يستخدم strcspn لإزالة حرف السطر الجديد من الإدخال.
9. دالة اختيار المسار الأساسي مع خيار "أخرى" (select_base_path_with_other)
const char* select_base_path_with_other(UserContext *user_ctx, const char *prompt) {
    // يسمح للمستخدم باختيار مسار أساسي أو تحديد دليل فرعي داخل مسار أساسي مسموح به
    // يعيد المسار المحدد أو NULL عند الفشل
}


الغرض: يوفر قائمة للمستخدم لاختيار الدليل الذي يريد تنفيذ العملية فيه.
الميزات:
يعرض المسارات الأساسية المسموح بها للاختيار.
يوفر خيار "أخرى" لتحديد دليل فرعي.
العملية:
عرض الخيارات: يعرض الأدلة المتاحة.
الحصول على اختيار المستخدم: يقرأ اختيار المستخدم.
التعامل مع خيار "أخرى": إذا تم اختيار "أخرى":
يطالب المستخدم باختيار دليل أساسي.
يطلب اسم الدليل الفرعي.
يتحقق من اسم الدليل الفرعي باستخدام sanitize_filename.
ينشئ المسار الكامل ويتحقق مما إذا كان الدليل موجودًا.
الاستخدام في الدوال الأخرى: يتم استدعاء هذه الدالة كلما احتاج البرنامج إلى تحديد الدليل لعملية ما.
10. دوال إجراءات المستخدم
هذه الدوال تنفذ عمليات الملفات المختلفة المتاحة للمستخدمين.

أ. عرض الملفات
void list_files(UserContext *user_ctx) {
    // يعرض جميع الملفات في الأدلة المسموح بها للمستخدم
}


الغرض: يعرض قائمة بالملفات التي يمكن للمستخدم الوصول إليها.
العملية:
يتنقل عبر المسارات الأساسية للمستخدم.
يستخدم أمر find عبر popen لعرض الملفات.
يعد ويعرض عدد الملفات في كل دليل.
ب. تغيير الأذونات
void change_permissions(UserContext *user_ctx) {
    // يسمح للمسؤول بتغيير أذونات الملفات
}


الغرض: يتيح للمسؤول تغيير أذونات الملفات.
العملية:
الحصول على اسم الملف: يطلب من المستخدم إدخال اسم الملف.
تنقية الإدخال: يتحقق من صحة اسم الملف.
اختيار الدليل: يستخدم select_base_path_with_other لاختيار دليل الملف.
إنشاء المسار الكامل: يبني المسار إلى الملف.
التحقق من صلاحية المسار: يتحقق مما إذا كان المسار صالحًا والملف موجودًا.
الحصول على الأذونات: يطلب الأذونات الجديدة (مثل 755).
تغيير الأذونات: يستخدم chmod لتطبيق الأذونات الجديدة.
ج. إنشاء دليل
void create_directory(UserContext *user_ctx) {
    // يسمح للمستخدم بإنشاء دليل جديد داخل المسارات المسموح بها
}


العملية:
الحصول على اسم الدليل: يطلب من المستخدم اسم الدليل.
تنقية الإدخال: يتحقق من صحة اسم الدليل.
اختيار المسار الأساسي: يستدعي select_base_path_with_other لاختيار مكان إنشاء الدليل.
إنشاء المسار الكامل: يبني المسار الكامل.
التحقق من صلاحية المسار: يتأكد من أن المسار صالح والدليل غير موجود.
إنشاء الدليل: يستخدم mkdir لإنشاء الدليل.
د. حذف دليل
void delete_directory(UserContext *user_ctx) {
    // يسمح للمستخدم بحذف دليل داخل المسارات المسموح بها
}


العملية:
مشابهة لـ create_directory، ولكن يستخدم system("rm -rf ...") لإزالة الدليل.
ملاحظة: استخدام rm -rf يمكن أن يكون خطيرًا؛ من الأفضل استخدام rmdir للأدلة الفارغة أو remove بشكل متكرر.
هـ. إنشاء ملف
void create_file(UserContext *user_ctx) {
    // يسمح للمستخدم بإنشاء ملف جديد داخل المسارات المسموح بها
}


العملية:
الحصول على اسم الملف: يطلب اسم الملف.
تنقية الإدخال: يتحقق من صحة اسم الملف.
اختيار المسار الأساسي: يختار مكان إنشاء الملف.
إنشاء المسار الكامل: يبني المسار الكامل.
التحقق من صلاحية المسار: يتحقق من أن الملف غير موجود.
إنشاء الملف: يستخدم system("touch ...") لإنشاء ملف فارغ.
ملاحظة: من الأفضل استخدام fopen لإنشاء ملف.
و. حذف ملف
void delete_file(UserContext *user_ctx) {
    // يسمح للمستخدم بحذف ملف داخل المسارات المسموح بها
}


العملية:
مشابهة لـ create_file، ولكن يحذف الملف باستخدام system("rm ...").
ملاحظة: من الأفضل استخدام remove من stdio.h.
ز. إنشاء رابط رمزي
void create_symbolic_link(UserContext *user_ctx) {
    // يسمح للمسؤول بإنشاء رابط رمزي
}


العملية:
الحصول على أسماء الهدف والرابط: يطلب الملف الهدف واسم الرابط.
تنقية الإدخالات: يتحقق من صحة كلا الاسمين.
اختيار الأدلة: يختار الأدلة للهدف والرابط.
إنشاء المسارات: يبني المسارات الكاملة.
التحقق من صلاحية المسارات: يتحقق من الصلاحية والوجود.
إنشاء الرابط: يستخدم system("ln -s ...") لإنشاء الرابط الرمزي.
ح. نسخ ملف
void copy_file(UserContext *user_ctx) {
    // يسمح للمستخدم بنسخ ملف داخل المسارات المسموح بها
}


العملية:
مشابهة للدوال السابقة ولكن يستخدم system("cp ...") لنسخ الملف.
التحقق:
يتم التحقق من كلا المسارين المصدر والوجهة.
وجود الملف المصدر.
ط. نقل ملف
void move_file(UserContext *user_ctx) {
    // يسمح للمستخدم بنقل ملف داخل المسارات المسموح بها
}


العملية:
مشابهة لنسخ الملف، ولكن يستخدم system("mv ...").
ملاحظة: ينقل الملف إلى موقع جديد، مما قد يغير مساره.
ي. إضافة نص إلى ملف
void append_to_file(UserContext *user_ctx) {
    // يسمح للمستخدم بإضافة نص إلى ملف
}


العملية:
الحصول على اسم الملف: يطلب الملف لإضافة نص إليه.
تنقية الإدخال: يتحقق من صحة اسم الملف.
اختيار المسار الأساسي: يختار مكان الملف.
الحصول على النص: يطلب من المستخدم النص لإضافته.
هروب الأحرف الخاصة: يضمن أن النص لن يكسر أمر الصدفة.
إضافة النص: يستخدم system("echo ... >> file") لإضافة النص.
ملاحظة: استخدام fopen في وضع الإلحاق "a" أكثر أمانًا وكفاءة.
ك. عرض محتوى ملف
void view_file_content(UserContext *user_ctx) {
    // يسمح للمستخدم بعرض محتوى ملف
}


العملية:
الحصول على اسم الملف: يطلب الملف لعرضه.
تنقية الإدخال: يتحقق من صحة اسم الملف.
اختيار المسار الأساسي: يختار دليل الملف.
اختيار خيار العرض: الملف بالكامل، أو البداية، أو النهاية.
عرض المحتوى: يستخدم system("cat"، "head"، أو "tail") لعرض محتوى الملف.
ل. البحث عن ملف
void find_file(UserContext *user_ctx) {
    // يسمح للمستخدم بالبحث عن ملفات تطابق نمطًا معينًا
}


العملية:
يطلب نمط اسم الملف (مثل "*.txt").
يبحث داخل الأدلة المسموح بها باستخدام system("find ...").
م. البحث في المحتوى
void search_content(UserContext *user_ctx) {
    // يسمح للمستخدم بالبحث عن كلمة مفتاحية داخل الملفات
}


العملية:
يطلب كلمة مفتاحية.
يبحث داخل الأدلة المسموح بها باستخدام system("grep -r ...").
11. دوال إدارة الأسماء المستعارة
هذه الدوال تسمح للمستخدمين بتعيين واستخدام الأسماء المستعارة للأوامر، مما يوفر الوقت على المهام المتكررة.

أ. تعيين اسم مستعار
void set_alias(UserContext *user_ctx) {
    // يسمح للمستخدم بإنشاء اسم مستعار لأمر
}


العملية:
التحقق من توفر الأسماء المستعارة: يتأكد من أن المستخدم يمكنه استخدام الأسماء المستعارة.
الحصول على اسم المستعار والأمر: يطلب كلاهما.
تخزين الاسم المستعار: يضيف الاسم المستعار إلى مصفوفة الأسماء المستعارة للمستخدم إذا كان هناك مساحة.
ب. استخدام اسم مستعار
void use_alias(UserContext *user_ctx) {
    // يسمح للمستخدم بتنفيذ أمر مرتبط باسم مستعار
}


العملية:
الحصول على اسم المستعار: يطلب الاسم المستعار للاستخدام.
البحث عن الاسم المستعار: يبحث في مصفوفة الأسماء المستعارة.
تنفيذ الأمر: يستدعي الدالة المقابلة بناءً على سلسلة الأمر.
12. دالة تسجيل الدخول
int login_user() {
    // يطلب من المستخدم اسم المستخدم وكلمة المرور
    // يتحقق من صحة البيانات
    // يعيد 1 عند النجاح، 0 عند الفشل
}


الغرض: يحاكي عملية تسجيل الدخول.
التنفيذ الحالي:
يقبل فقط اسم المستخدم "ali" وكلمة المرور "1".
ملاحظة: بالنسبة لتطبيق حقيقي، يجب أن يكون التعامل مع كلمات المرور أكثر أمانًا ويدعم عدة مستخدمين.
13. دالة القائمة الرئيسية
void main_menu(UserContext *user_ctx) {
    // يعرض الخيارات بناءً على نوع المستخدم ويتعامل مع اختيارات المستخدم
}


الغرض: يوفر حلقة التفاعل الرئيسية للمستخدم بعد تسجيل الدخول.
العملية:
عرض القائمة: يعرض الخيارات بناءً على user_type.
الحصول على اختيار المستخدم: يقرأ الخيار المحدد.
تنفيذ الإجراء: يستدعي الدالة المقابلة.
تكرار: يستمر حتى يختار المستخدم تسجيل الخروج.
14. دالة اختيار نوع المستخدم
void select_user_type() {
    // يسمح للمستخدم باختيار نوع المستخدم ويبدأ الجلسة
}


الغرض: يوفر القائمة الأولية لاختيار نوع المستخدم.
العملية:
عرض الخيارات: يعرض أنواع المستخدمين المتاحة.
الحصول على اختيار المستخدم: يقرأ الاختيار.
إعداد سياق المستخدم: يهيئ UserContext بناءً على نوع المستخدم المختار.
تسجيل الدخول وبدء الجلسة: يستدعي login_user و main_menu.
تكرار: يعود إلى اختيار نوع المستخدم بعد تسجيل الخروج أو فشل تسجيل الدخول.
*/