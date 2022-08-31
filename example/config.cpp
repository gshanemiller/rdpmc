#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

// Purpose: make & pretty print Intel PMU programmable counter configuration.
// Use with 'doc/pmu.md' which has background and references to this structure
// and lift event/umask/cmask from https://perfmon-events.intel.com/

int csvFormat = 0;
int csvHeader = 0;

struct CounterConfig {
  u_int32_t event : 8;
  u_int32_t umask : 8;
  u_int32_t usr   : 1;
  u_int32_t os    : 1;
  u_int32_t edge  : 1;
  u_int32_t pinc  : 1;
  u_int32_t intrup: 1;
  u_int32_t any   : 1;
  u_int32_t enab  : 1;
  u_int32_t invrt : 1;
  u_int32_t cmask : 8;
};

typedef union {
  struct CounterConfig field;
  u_int32_t            value;
} Config;


void prettyPrintCsv(const char *name, const char *description, const Config& cfg) {
  if (!csvHeader) {
    printf("\"decValue\",\"hexValue\",\"name\", \"description\"\n");
    csvHeader = 1;
  }

  printf("%u,%x,\"%s\",\"%s\"\n", cfg.value, cfg.value, name, description);
}

void prettyPrintDft(const char *name, const char *description, const Config& cfg) {
  printf("'%s' : '%s' <- 0x%08x\n", description, name, cfg.value);
  printf("------------------------------------------------------\n");
  printf("event : 0x%02x\n", cfg.field.event);
  printf("umask : 0x%02x\n", cfg.field.umask);
  printf("usr   : 0x%02x\n", cfg.field.usr);
  printf("os    : 0x%02x\n", cfg.field.os);
  printf("edge  : 0x%02x\n", cfg.field.edge);
  printf("pinc  : 0x%02x\n", cfg.field.pinc);
  printf("intrup: 0x%02x\n", cfg.field.intrup);
  printf("any   : 0x%02x\n", cfg.field.any);
  printf("enab  : 0x%02x\n", cfg.field.enab);
  printf("invrt : 0x%02x\n", cfg.field.invrt);
  printf("cmask : 0x%02x\n", cfg.field.cmask);
  printf("\n\n");
}

void prettyPrint(const char *name, const char *description, const Config& cfg) {
  if (csvFormat) {
      prettyPrintCsv(name, description, cfg);
  } else {
      prettyPrintDft(name, description, cfg);
  }
}

int main(int argc, char **argv __attribute__((unused))) {
  assert(sizeof(struct CounterConfig)==sizeof(u_int32_t));

  // Super cheap 'usage line'
  if (argc>1) {
    csvFormat = 1;
  }

  Config cfg;

  cfg.value = 0;
  cfg.field.event = 0x2e;
  cfg.field.umask = 0x41;
  cfg.field.usr   = 1;
  cfg.field.enab  = 1;
  prettyPrint("LONGEST_LAT_CACHE.MISS", "LLC cache misses", cfg);

  
  return 0;
}
