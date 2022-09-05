#include <intel_skylake_pmu.h>
#include <intel_pmu_stats.h>

#include <algorithm>

#include <time.h>
#include <unistd.h>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

PMU *pmu = 0;

// Hold values in configuration file if provided
std::vector <u_int64_t>   valu;
std::vector <std::string> name;
std::vector <std::string> desc;
std::vector <const char*> namePtrArray;
std::vector <const char*> descPtrArray;
std::vector <u_int64_t>   valuArray;

void test0() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  pmu->printSnapshot("test loop no memory accesses");
}

void test1() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  pmu->printSnapshot("test loop no memory accesses");
}

void test2(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed, however, not randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  pmu->printSnapshot("test loop accessing memory non-randomly");
}

void test3(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  pmu->printSnapshot("test loop accessing memory randomly");
}

void test4() {
  pmu->reset();
  pmu->start();

  // No memory accessed
  unsigned s=0;
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    s+=1;
  }

  pmu->printSnapshot("test loop no memory accesses");
}

void test5(int *ptr) {
  Intel::Stats stats;

  char desc[128];
  sprintf(desc, "Randomly accessed memory");

  timespec start, end;
 
  for (unsigned runs=0; runs<10; ++runs) {
    pmu->reset();
    timespec_get(&start, TIME_UTC);                                                                                   
    pmu->start();

    // Memory heavily accessed randomly
    for (volatile int i=0; i<MAX_INTEGERS; ++i) {
      long idx = random() % MAX_INTEGERS;
      *(ptr+idx) = 0xdeadbeef;
    }

    timespec_get(&end, TIME_UTC);                                                                                   
  
    stats.record(desc, MAX_INTEGERS, start, end, *pmu);
  }

  stats.legend(*pmu);
  stats.dump(*pmu);
  stats.dumpScaled(*pmu);
}

void usageAndExit() {
  printf("usage: ./example.tsk [-f <file> -p <name>]\n");
  printf("\n");
  printf("  where:\n");
  printf("       -f <file> contains programmable PMU configs in CSV format as per example/config.cpp\n");
  printf("       -p <name> configure and run 'name' during test. repeat for more counters up to limit\n");
  printf("          specified 'name' must occur in name column of 'file'\n");
  printf("\n");
  printf("  Fixed counters are always run. If no arguments are provided test runs with default\n");
  printf("  'k_DEFAULT_SKYLAKE_CONFIG_0' set.\n");
  exit(2);
}

void processCommandLine(int argc, char **argv) {
  int opt;
  int line = 0;
  int count = 0;
  FILE *fid = 0;
  char buffer[1024];

  while ((opt = getopt(argc, argv, "f:p:")) != -1) {
    switch (opt) {
    case 'f':
    {
      if (fid!=0) {
        // file already open
        fprintf(stderr, "-f specified two or more times\n");
        usageAndExit();
      }
      if ((fid = fopen(optarg, "r"))==0) {
        // file not found
        fprintf(stderr, "file '%s' could not be opened\n", optarg);
        usageAndExit();
      }
      // NB: this simplistic parser does not expect commas
      // in line except as CSV file delimiters.
      const char *delimiters = ",";
      while (!feof(fid)) {
        if (fgets(buffer, sizeof(buffer), fid)==0) {
          continue;
        }
        if (++line==1) {
          // skip header
          continue;
        }
        char *decValue = strtok(buffer, delimiters);
        char *ignore   = strtok(0, delimiters);             // hexValue - don't need
        char *nameCstr = strtok(0, delimiters);
        char *descCstr = strtok(0, delimiters);
        if (decValue==0 || ignore==0 || nameCstr==0 || descCstr==0) {
          // unexpected line
          fprintf(stderr, "could not parse line %d of file '%s'\n", line, optarg);
          fclose(fid);
          usageAndExit();
        }
        valu.push_back((u_int64_t)strtol(decValue, 0, 10));
        // Remove any quotes in name
        std::string tmp(nameCstr);
        tmp.erase(std::remove(tmp.begin(), tmp.end(), '"'), tmp.end());
        name.push_back(tmp);
        // Remove any quotes in desc
        tmp = descCstr;
        tmp.erase(std::remove(tmp.begin(), tmp.end(), '"'), tmp.end());
        // Remove newline in desc
        tmp.erase(std::remove(tmp.begin(), tmp.end(), '\n'), tmp.end());
        desc.push_back(tmp);
      }
      fclose(fid);
    }
    break;
    case 'p':
    {
      if (fid==0) {
        fprintf(stderr, "specify -f before -p arguments\n");
        usageAndExit();
      }
      int found=0;
      std::string match(optarg);
      for (unsigned i=0; i<name.size(); ++i) {
        if (match==name[i]) {
          found = 1;
          // Given selected counter named by 'match' mnemoic shorthand name 'P<n>'
          sprintf(buffer, "P%d", count++);
          namePtrArray.push_back(buffer);
          descPtrArray.push_back(desc[i].c_str());
          valuArray.push_back(valu[i]);
          fprintf(stdout, "added programmable counter '%s': '%s' -> 0x%08lx\n", name[i].c_str(), desc[i].c_str(), valu[i]);
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "could not find '%s' in specified -f file\n", optarg);
        usageAndExit();
      }
    }
    break;
    default:
      usageAndExit();
    }
  }

  if (fid!=0 && valuArray.size()) {
    pmu = new PMU(false, valuArray.size(), valuArray.data(), namePtrArray.data(), descPtrArray.data());
  } else {
    pmu = new PMU(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  }

  if (pmu==0) {
    fprintf(stderr, "unexpected error creating PMU object\n");
    usageAndExit();
  }
}

int main(int argc, char **argv) {
  processCommandLine(argc, argv);

  int *ptr = (int*)malloc(sizeof(int)*MAX_INTEGERS);
  if (ptr==0) {
      fprintf(stderr, "Error: memory allocation failed\n");
      return 1;
  }

  test0();
  test1();
  test2(ptr);
  test3(ptr);
  test4();
  test5(ptr);

  free(ptr);
  ptr=0;
  delete pmu;

  return 0;
}
