#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { UNIT_AUTO, UNIT_B, UNIT_K, UNIT_M, UNIT_G } unit_t;

typedef struct {
    unit_t unit;
    int human;
    int show_total;
    int interval;
    int count;
    int show_version;
} options_t;

void print_version() {
    printf("winfree 1.0 - GNU free for Windows\n");
}

void print_usage() {
    printf("Usage: winfree [options]\n"
           "  -b, --bytes          show output in bytes\n"
           "  -k, --kibi           show output in KiB\n"
           "  -m, --mebi           show output in MiB\n"
           "  -g, --gibi           show output in GiB\n"
           "  -h, --human          human readable units\n"
           "  -t, --total          show total line\n"
           "  -s, --seconds N      repeat display, every N seconds\n"
           "  -c, --count   N      repeat display N times\n"
           "  -V, --version        show version\n"
           "  --help               show this help\n");
}

unit_t detect_default_unit() {
    return UNIT_M; // Default to MiB for Windows, like 'free -m'
}

void parse_args(int argc, char *argv[], options_t *opt) {
    opt->unit = detect_default_unit();
    opt->human = 0;
    opt->show_total = 0;
    opt->interval = 0;
    opt->count = -1; // Unlimited
    opt->show_version = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytes")) opt->unit = UNIT_B;
        else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--kibi")) opt->unit = UNIT_K;
        else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mebi")) opt->unit = UNIT_M;
        else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--gibi")) opt->unit = UNIT_G;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--human")) opt->human = 1;
        else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--total")) opt->show_total = 1;
        else if ((!strcmp(argv[i], "-s") || !strcmp(argv[i], "--seconds")) && i+1 < argc) {
            opt->interval = atoi(argv[++i]);
        }
        else if ((!strcmp(argv[i], "-c") || !strcmp(argv[i], "--count")) && i+1 < argc) {
            opt->count = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) opt->show_version = 1;
        else if (!strcmp(argv[i], "--help")) {
            print_usage();
            exit(0);
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage();
            exit(1);
        }
    }
}

const char *unit_name(unit_t unit) {
    switch(unit) {
        case UNIT_B: return "B";
        case UNIT_K: return "KiB";
        case UNIT_M: return "MiB";
        case UNIT_G: return "GiB";
        default:     return "";
    }
}

double conv(DWORDLONG val, unit_t unit) {
    switch(unit) {
        case UNIT_B: return (double)val;
        case UNIT_K: return (double)val / 1024.0;
        case UNIT_M: return (double)val / (1024.0 * 1024.0);
        case UNIT_G: return (double)val / (1024.0 * 1024.0 * 1024.0);
        default:     return (double)val;
    }
}

void humanize(double val, char *buf, size_t sz) {
    const char *units[] = {"B", "K", "M", "G", "T"};
    int i = 0;
    while (val >= 1024 && i < 4) {
        val /= 1024;
        ++i;
    }
    snprintf(buf, sz, "%.1f%s", val, units[i]);
}

void print_mem_status(options_t *opt) {
    MEMORYSTATUSEX s;
    s.dwLength = sizeof(s);
    GlobalMemoryStatusEx(&s);

    DWORDLONG mem_total = s.ullTotalPhys;
    DWORDLONG mem_free = s.ullAvailPhys;
    DWORDLONG mem_used = mem_total - mem_free;

    DWORDLONG swap_total = s.ullTotalPageFile;
    DWORDLONG swap_free = s.ullAvailPageFile;
    DWORDLONG swap_used = swap_total - swap_free;

    double mem_total_out = conv(mem_total, opt->unit);
    double mem_used_out = conv(mem_used, opt->unit);
    double mem_free_out = conv(mem_free, opt->unit);

    double swap_total_out = conv(swap_total, opt->unit);
    double swap_used_out = conv(swap_used, opt->unit);
    double swap_free_out = conv(swap_free, opt->unit);

    char mem_total_h[16], mem_used_h[16], mem_free_h[16];
    char swap_total_h[16], swap_used_h[16], swap_free_h[16];
    if (opt->human) {
        humanize(mem_total, mem_total_h, sizeof(mem_total_h));
        humanize(mem_used, mem_used_h, sizeof(mem_used_h));
        humanize(mem_free, mem_free_h, sizeof(mem_free_h));
        humanize(swap_total, swap_total_h, sizeof(swap_total_h));
        humanize(swap_used, swap_used_h, sizeof(swap_used_h));
        humanize(swap_free, swap_free_h, sizeof(swap_free_h));
    }

    printf("              total        used        free\n");
    if (opt->human)
        printf("Mem:   %12s %12s %12s\n", mem_total_h, mem_used_h, mem_free_h);
    else
        printf("Mem:   %12.0f %12.0f %12.0f %s\n",
            mem_total_out, mem_used_out, mem_free_out, unit_name(opt->unit));
    if (opt->human)
        printf("Swap:  %12s %12s %12s\n", swap_total_h, swap_used_h, swap_free_h);
    else
        printf("Swap:  %12.0f %12.0f %12.0f %s\n",
            swap_total_out, swap_used_out, swap_free_out, unit_name(opt->unit));

    if (opt->show_total) {
        double total_all = mem_total_out + swap_total_out;
        double used_all  = mem_used_out + swap_used_out;
        double free_all  = mem_free_out + swap_free_out;
        char total_all_h[16], used_all_h[16], free_all_h[16];
        if (opt->human) {
            humanize(mem_total + swap_total, total_all_h, sizeof(total_all_h));
            humanize(mem_used + swap_used, used_all_h, sizeof(used_all_h));
            humanize(mem_free + swap_free, free_all_h, sizeof(free_all_h));
            printf("Total: %12s %12s %12s\n", total_all_h, used_all_h, free_all_h);
        } else {
            printf("Total: %12.0f %12.0f %12.0f %s\n", total_all, used_all, free_all, unit_name(opt->unit));
        }
    }
}

int main(int argc, char *argv[]) {
    options_t opt;
    parse_args(argc, argv, &opt);

    if (opt.show_version) {
        print_version();
        return 0;
    }

    int n = 0;
    do {
        if (n > 0 && opt.interval > 0) Sleep(opt.interval * 1000);
        print_mem_status(&opt);
        ++n;
        if (opt.count > 0 && n >= opt.count) break;
        if (opt.interval <= 0) break;
    } while (1);

    return 0;
}
