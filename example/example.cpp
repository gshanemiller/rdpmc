#include <intel_skylake_pmu->h>
#include <unistd.h>

#include <algorithm>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

PMU *pmu = 0;

void test0() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  pmu->print("test loop no memory accesses");
}

void test1() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  pmu->print("test loop no memory accesses");
}

void test2(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed, however, not randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  pmu->print("test loop accessing memory non-randomly");
}

void test3(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  pmu->print("test loop accessing memory randomly");
}

void test4() {
  pmu->reset();
  pmu->start();

  // No memory accessed
  unsigned s=0;
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    s+=1;
  }

  pmu->print("test loop no memory accesses");
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

void processCommandLine() {
  int opt;
  int line = 0;
  FILE *fid = 0;
  char buffer[1024];

  std::vector <u_int64_t>   valu;
  std::vector <std::string> name;
  std::vector <std::string> desc;

  std::vector <std::string*> namePtrArray;
  std::vector <std::string*> descPtrArray;
  std::vector <u_int64_t>    valuArray;

  while ((opt = getopt(argc, argv, "f:p:")) != -1) {
  switch (opt) {
    case 'f':
      if (fid!=0) {
        // file already open
        fprintf(stderr, "-f specified two or more times\n");
        usageAndExit();
      }
      if ((fid = fopen(optarg))==0) {
        // file not found
        fprintf(stderr, "file '%s' could not be opened\n", optarg);
        usageAndExit();
      }
      // NB: this simplistic parser does not expect commas
      // in line except as CSV file delimiters.
      const char *delimiters = ",";
      while (!feof(fid)) {
        fgets(buffer, sizeof(buffer), fid);
        if (++line==1) {
          // skip header
          continue;
        }
        char *decValue = strtok(buffer, delimiters);
        char *skip = strtok(0, delimiters);
        char *name = strtok(0, delimiters);
        char *desc = strtok(0, delimiters);
        if (decValue==0 || hexValue==0 || name==0 || desc==0) {
          // unexpected line
          fprintf(stderr, "could not parse line %d of file '%s'\n", line, optarg);
          fclose(fid);
          usageAndExit();
        }
        valu.push_back((u_int64_t)strtol(decValue, 0, 10));
        std::string tmp(name);
        std::remove(tmp.begin(), tmp.end(), '"'); 
        name.push_back(name);
        tmp = desc;
        std::remove(tmp.begin(), tmp.end(), '"'); 
        desc.push_back(desc);
      }
      break;
    case 'p':
      if (fid==0) {
        fprintf(stderr, "specify -f before -p arguments);
        usageAndExit();
      }
      int found=0;
      std::string match(optarg);
      for (unsigned i=0; i<name.size(); ++i) {
        if (match==name[i]) {
          found = 1;
          namePtrArray.push_back(name[i].c_str());
          descPtrArray.push_back(desc[i].c_str());
          valuArray.push_back(valu[i]);
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "could not find '%s' in specified -f file\n", optarg);
        usageAndExit();
      }
      break;
    default:
      usageAndExit();
  }
}

int main() {
  processCommandLine();

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

  free(ptr);
  ptr=0;
  delete pmu;

  return 0;
}
