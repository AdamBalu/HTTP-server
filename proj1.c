#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void free_func(char **s) {
    if (*s != NULL) { 
        free(*s);
        *s = NULL;
    }
}

void free_func_i(int **s) {
    if (*s != NULL) { 
        free(*s);
        *s = NULL;
    }
}

char *strip_newline(char **to_strip) {
    char *newline;
    newline = strstr(*to_strip, "\n");
    if (newline != NULL)
        strncpy(newline, "\0", 1);
}

/**
 * @brief splits CPU values by delimiter and returns list of these values
 * 
 * @param a_str CPU values
 * @param a_delim delimiter
 * @return int* list of CPU values
 */
int *str_split(char *a_str, const char a_delim) {
    int *result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_space = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // count how many elements will be extracted
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_space = tmp;
        }
        tmp++;
    }
    count += last_space < (a_str + strlen(a_str) - 1);

    count++;

    result = malloc(sizeof(int) * count);
    if (result) {
        size_t idx = 0;
        char *token = strtok(a_str, delim);
        while (token)
        {
            int token_int = strtol(token, NULL, 10);
            *(result + idx++) = token_int;
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}

char *format_values(char *cpu_values) {
    // remove newline character at the end
    strip_newline(&cpu_values);
    
    char *final_str = cpu_values;
    // get rid of 'cpu  ' in the beginning
    for (int i = 0; i < 5; i++) {
        final_str++;
    }
    return final_str;
}

/**
 * @brief get a list of CPU values from /proc/stat
 * 
 * @return int* values
 */
int *get_values() {
    FILE *stats;
    stats = fopen("/proc/stat", "r");
    char cpu_values_raw[100];
    memset(cpu_values_raw, 0, 100);

    fgets(cpu_values_raw, 100, stats);
    
    char *cpu_values_form;

    cpu_values_form = format_values(cpu_values_raw);

    // split values by delimiter ' '
    const char delim = ' ';
    int *values = str_split(cpu_values_form, delim);
    fclose(stats);
    return values;
}

float cpu_usage() {
    int *prev;
    int *curr;

    prev = get_values();

    sleep(1);

    curr = get_values();

    // For example:
    //     user    nice    system   idle      iowait  irq     softirq  steal   guest   guest_nice
    // cpu 74608   2520    24433    1117073   6176    4054    0        0       0       0
    //     prev[0] prev[1] prev[2]  prev[3]   prev[4] prev[5] prev[6]  prev[7] prev[8] prev[9]
    //
    // same applies for curr[0-9]

    // PrevIdle = previdle + previowait
    int prev_idle = prev[3] + prev[4];
    // Idle = idle + iowait
    int idle = curr[3] + curr[4];
    // PrevNonIdle = prevuser + prevnice + prevsystem + previrq + prevsoftirq + prevsteal
    int prev_non_idle = prev[0] + prev[1] + prev[2] + prev[5] + prev[6] + prev[7];
    // NonIdle =   user    + nice    + system  + irq     + softirq + steal
    int non_idle = curr[0] + curr[1] + curr[2] + curr[5] + curr[6] + curr[7];

    int prev_total = prev_idle + prev_non_idle;
    int total = idle + non_idle;
    int total_d = total - prev_total;
    int idle_d = idle - prev_idle;

    float cpu_percentage;
    if (total_d != 0) {
        cpu_percentage = (total_d - idle_d);
        cpu_percentage = cpu_percentage / total_d * 100;
    }
    else {
        cpu_percentage = 0;
    }

    free_func_i(&prev);
    free_func_i(&curr);
    return cpu_percentage;
}

void name_help(char **name, FILE *f) {
    *name = malloc(sizeof(char) * 100);
    fgets(*name, 100, f);
    strip_newline(name);
    fclose(f);
}

char *host_name(char **hostname) {
    FILE *f;
    f = fopen("/proc/sys/kernel/hostname", "r");
    name_help(hostname, f);
}

char *cpu_name(char **cpu) {
    FILE *f;
    f = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F\": \" '{print $2}'", "r");
    name_help(cpu, f);
}

int send_all(int client_socket, char *http_header, int request_len) {
    int sent = 0;
    ssize_t n;
    const char *p = http_header;
    while (request_len > 0) {
        n = send(client_socket, p, request_len, 0);
        if (n <= 0) return -1;
        p += n;
        request_len -= n;
    }
    return 0;
}

void concat_str(int *request_len, char **send_buffer, char *message) {
    *request_len += strlen(message) + 2;
    strcat(*send_buffer, message);
    strcat(*send_buffer, "\r\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }

    size_t port = strtol(argv[1], NULL, 10);
    float cpu_usg;
    char cpu_usg_buf[6];
    char *hostname;
    host_name(&hostname);

    char *cpu;
    cpu_name(&cpu);

    char *send_buffer = (char *) calloc(2048, sizeof(char));
    char http_req_ok[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
    char http_req_err[] = "HTTP/1.1 400 Bad Request\r\n\r\n";

    // // create a socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    // define the address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(server_socket, 5);

    int client_socket;

    while (1) {
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
            perror("socket");
            exit(1);
        }
        memset(send_buffer, 0, sizeof(send_buffer));

        char http_command[10000];
        int request_len = strlen(http_req_ok);

        // http request OK until proven otherwise
        strcat(send_buffer, http_req_ok);

        if ((recv(client_socket, &http_command, sizeof(http_command), 0)) <= 0) {
            perror("receive");
            exit(1);
        }

        // types of valid requests
        if (strstr(http_command, "GET /hostname") == &http_command[0]) {
            concat_str(&request_len, &send_buffer, hostname);
        }
        else if (strstr(http_command, "GET /cpu-name") == &http_command[0]) {
            concat_str(&request_len, &send_buffer, cpu);
        }
        else if (strstr(http_command, "GET /load") == &http_command[0]) {
            cpu_usg = cpu_usage();
            gcvt(cpu_usg, 2, cpu_usg_buf);
            strcat(cpu_usg_buf, "\%");
            concat_str(&request_len, &send_buffer, cpu_usg_buf);
        }
        else {
            // clear the output buffer and set error Bad Request 400
            request_len = strlen(http_req_err);
            memset(send_buffer, 0, 2048);
            concat_str(&request_len, &send_buffer, http_req_err);
        }
        send_all(client_socket, send_buffer, request_len);
        close(client_socket);
    }

    free_func(&hostname);
    free_func(&cpu);
    free_func(&send_buffer);

    close(server_socket);
    return 0;
}
