#include <intel_pmu_stats.h>
#include <intel_skylake_pmu.h>

void Intel::Stats::legend(const Intel::SkyLake::PMU& pmu) const {
  printf("%-3s [%-60s]\n", "C0", "rdtsc cycles: use with F2");

  printf("%-3s [%-60s]\n", pmu.fixedMnemonic()[0].c_str(), pmu.fixedDescription()[0].c_str());
  printf("%-3s [%-60s]\n", pmu.fixedMnemonic()[1].c_str(), pmu.fixedDescription()[1].c_str());
  printf("%-3s [%-60s]\n", pmu.fixedMnemonic()[2].c_str(), pmu.fixedDescription()[2].c_str());

  for (unsigned i=0; i<pmu.programmableCounterDefined(); ++i) {
    printf("%-3s [%-60s]\n", pmu.progMnemonic()[i].c_str(), pmu.progDescription()[i].c_str());
  }

  char buffer[128];

  sprintf(buffer, "nanoseconds elapsed");
  printf("%-3s [%-60s]\n", "NS", buffer);

  sprintf(buffer, "nanoseconds per iteration");
  printf("%-3s [%-60s]\n", "NSI", buffer);

  sprintf(buffer, "iterations per second");
  printf("%-3s [%-60s]\n", "OPS", buffer);

  sprintf(buffer, "iterations");
  printf("%-3s [%-60s]\n", "N", buffer);
}

void Intel::Stats::dump(const Intel::SkyLake::PMU& pmu) const {
  for (unsigned i=0; i<d_description.size(); ++i) {
    printf("Result Set %d: %s: Intel::Skylake CPU HW core %d\n", i, d_description[i].c_str(), pmu.core());

    printf(  "%-3s: [%-60s] value: %lu\n", "C0", "rdtsc cycles: use with F2", d_rdstc[i]); 

    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[0].c_str(), pmu.fixedDescription()[0].c_str(), d_fixedCntr0[0]);
    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[1].c_str(), pmu.fixedDescription()[1].c_str(), d_fixedCntr0[1]);
    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[2].c_str(), pmu.fixedDescription()[2].c_str(), d_fixedCntr0[2]);

    if (pmu.programmableCounterDefined()>0) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[0].c_str(), pmu.progDescription()[0].c_str(), d_progmCntr0[i]);
    }
    if (pmu.programmableCounterDefined()>1) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[1].c_str(), pmu.progDescription()[1].c_str(), d_progmCntr1[i]);
    }
    if (pmu.programmableCounterDefined()>2) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[2].c_str(), pmu.progDescription()[2].c_str(), d_progmCntr2[i]);
    }
    if (pmu.programmableCounterDefined()>3) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[3].c_str(), pmu.progDescription()[3].c_str(), d_progmCntr3[i]);
    }
    if (pmu.programmableCounterDefined()>4) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[4].c_str(), pmu.progDescription()[4].c_str(), d_progmCntr4[i]);
    }
    if (pmu.programmableCounterDefined()>5) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[5].c_str(), pmu.progDescription()[5].c_str(), d_progmCntr5[i]);
    }
    if (pmu.programmableCounterDefined()>6) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[6].c_str(), pmu.progDescription()[6].c_str(), d_progmCntr6[i]);
    }
    if (pmu.programmableCounterDefined()>7) {
      printf(  "%-3s: [%-60s] value: %lu\n", pmu.progMnemonic()[7].c_str(), pmu.progDescription()[7].c_str(), d_progmCntr7[i]);
    }

    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "NS", "nanoseconds elapsed", d_elapsedNs[i]);
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "NSI", "nanoseconds per iteration",  (double)d_elapsedNs[i]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "OPS", "operations per second", (double)1000000000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %lu\n",  "N", "iterations", d_itertions[i]);
  }
}

void Intel::Stats::dumpScaled(const Intel::SkyLake::PMU& pmu) const {
  for (unsigned i=0; i<d_description.size(); ++i) {
    printf("Scaled Result Set %d: %s: Intel::Skylake CPU HW core %d\n", i, d_description[i].c_str(), pmu.core());

    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "C0", "rdtsc cycles: use with F2", (double)d_rdstc[i]/(double)d_itertions[i]); 

    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[0].c_str(), pmu.fixedDescription()[0].c_str(), (double)d_fixedCntr0[0]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[1].c_str(), pmu.fixedDescription()[1].c_str(), (double)d_fixedCntr0[1]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[2].c_str(), pmu.fixedDescription()[2].c_str(), (double)d_fixedCntr0[2]/(double)d_itertions[i]);

    if (pmu.programmableCounterDefined()>0) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[0].c_str(), pmu.progDescription()[0].c_str(), (double)d_progmCntr0[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>1) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[1].c_str(), pmu.progDescription()[1].c_str(), (double)d_progmCntr1[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>2) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[2].c_str(), pmu.progDescription()[2].c_str(), (double)d_progmCntr2[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>3) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[3].c_str(), pmu.progDescription()[3].c_str(), (double)d_progmCntr3[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>4) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[4].c_str(), pmu.progDescription()[4].c_str(), (double)d_progmCntr4[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>5) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[5].c_str(), pmu.progDescription()[5].c_str(), (double)d_progmCntr5[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>6) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[6].c_str(), pmu.progDescription()[6].c_str(), (double)d_progmCntr6[i]/(double)d_itertions[i]);
    }
    if (pmu.programmableCounterDefined()>7) {
      printf(  "%-3s: [%-60s] value: %-11.5lf\n", pmu.progMnemonic()[7].c_str(), pmu.progDescription()[7].c_str(), (double)d_progmCntr7[i]/(double)d_itertions[i]);
    }

    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "NS", "nanoseconds elapsed", d_elapsedNs[i]);
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "NSI", "nanoseconds per iteration", (double)d_elapsedNs[i]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "OPS", "operations per second", (double)1000000000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %lu\n",  "N", "iterations", d_itertions[i]);
  }
}

/*
void Intel::Stats::summary(const Intel::SkyLake::PMU& pmu) const {
}
*/

void Intel::Stats::record(
    const char *description,                                                                            
    unsigned long iterations,                                                                           
    timespec start,                                          
    timespec end,                                            
    const Intel::SkyLake::PMU& pmu)
{
  u_int64_t ts = pmu.timeStampCounter();                                                                                    

  u_int64_t fixed[3];                                                                                    
  fixed[0] = pmu.fixedCounterValue(0);                                                                                    
  fixed[1] = pmu.fixedCounterValue(1);                                                                                    
  fixed[2] = pmu.fixedCounterValue(2);                                                                                    

  u_int64_t prog[8];                                                                           
  if (pmu.programmableCounterDefined()>0) {
    prog[0] = pmu.programmableCounterValue(0);                                                                              
  }
  if (pmu.programmableCounterDefined()>1) {
    prog[1] = pmu.programmableCounterValue(1);                                                                              
  }
  if (pmu.programmableCounterDefined()>2) {
    prog[2] = pmu.programmableCounterValue(2);                                                                              
  }
  if (pmu.programmableCounterDefined()>3) {
    prog[3] = pmu.programmableCounterValue(3);                                                                              
  }
  if (pmu.programmableCounterDefined()>4) {
    prog[4] = pmu.programmableCounterValue(4);                                                                              
  }
  if (pmu.programmableCounterDefined()>5) {
    prog[5] = pmu.programmableCounterValue(5);                                                                              
  }
  if (pmu.programmableCounterDefined()>6) {
    prog[6] = pmu.programmableCounterValue(6);                                                                              
  }
  if (pmu.programmableCounterDefined()>7) {
    prog[7] = pmu.programmableCounterValue(7);                                                                              
  }

  d_description.push_back(description);
  d_itertions.push_back(iterations);

  d_rdstc.push_back(ts - pmu.startTimeStampCounter());

  d_fixedCntr0.push_back(fixed[0]);
  d_fixedCntr1.push_back(fixed[1]);
  d_fixedCntr2.push_back(fixed[2]);

  if (pmu.programmableCounterDefined()>0) {
    d_progmCntr0.push_back(prog[0]);
  }
  if (pmu.programmableCounterDefined()>1) {
    d_progmCntr1.push_back(prog[1]);
  }
  if (pmu.programmableCounterDefined()>2) {
    d_progmCntr2.push_back(prog[2]);
  }
  if (pmu.programmableCounterDefined()>3) {
    d_progmCntr3.push_back(prog[3]);
  }
  if (pmu.programmableCounterDefined()>4) {
    d_progmCntr4.push_back(prog[4]);
  }
  if (pmu.programmableCounterDefined()>5) {
    d_progmCntr5.push_back(prog[5]);
  }
  if (pmu.programmableCounterDefined()>6) {
    d_progmCntr6.push_back(prog[6]);
  }
  if (pmu.programmableCounterDefined()>7) {
    d_progmCntr7.push_back(prog[7]);
  }

  double elapsedNs = (double)end.tv_sec*1000000000.0+(double)end.tv_nsec -
                     ((double)start.tv_sec*1000000000.0+(double)start.tv_nsec);
  d_elapsedNs.push_back(elapsedNs);
}
