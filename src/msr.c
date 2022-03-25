#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>
#include <msr.h>

const unsigned int highbit = 63, lowbit = 0;                                                                                  

int rdmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t *value) {
	u_int64_t data;
	int fd;
	char *pat;
	int width;
	char msr_file_name[64];
	unsigned int bits;

  printf("read PMU MSR cpu %d 0x%x\n", cpu, reg);

	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
	fd = open(msr_file_name, O_RDONLY);
	if (fd < 0) {
		if (errno == ENXIO) {
			fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
      return 2;
		} else if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", cpu);
	    return 3;	
		} else {
			perror("rdmsr: open");
      return 4;
		}
	}

	if (pread(fd, &data, sizeof data, reg) != sizeof data) {
		if (errno == EIO) {
			fprintf(stderr, "rdmsr: CPU %d cannot read " "MSR 0x%08"PRIx32"\n", cpu, reg);
      return 5;
		} else {
			perror("rdmsr: pread");
      return 6;
		}
	}

	close(fd);

	bits = highbit - lowbit + 1;
	if (bits < 64) {
		/* Show only part of register */
		data >>= lowbit;
		data &= (1ULL << bits) - 1;
	}

  *value = data;

	return 0;
}

int wrmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t data) {
	int fd;
	char msr_file_name[64];

  printf("write PMU MSR cpu %d 0x%x 0x%lx\n", cpu, reg, data);

	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
	fd = open(msr_file_name, O_WRONLY);
	if (fd < 0) {
		if (errno == ENXIO) {
			fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
			return 2;
		} else if (errno == EIO) {
			fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n", cpu);
      return 3;
		} else {
			perror("wrmsr: open");
      return 4;
		}
	}

  if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
    if (errno == EIO) {
				fprintf(stderr, "wrmsr: CPU %d cannot set MSR " "0x%08"PRIx32" to 0x%016"PRIx64"\n", cpu, reg, data);
        return 5;
    } else {
      perror("wrmsr: pwrite");
      return 6;
    }
  }

	close(fd);

	return 0;
}
