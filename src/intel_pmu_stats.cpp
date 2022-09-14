#include <intel_pmu_stats.h>
#include <intel_skylake_pmu.h>
#include <assert.h>

void Intel::Stats::calcMinMaxAvgTime(const std::vector<double>& elapsedNs, const std::vector<u_int64_t>& iterations,
  double ns[3], double nsPerIter[3], double ops[3], double iters[3]) const {

  // Initialize
  for (unsigned i=0; i<3; ++i) {
    ns[i] = nsPerIter[i] = ops[i] = iters[i] = 0.0;
  }

  unsigned minIndex = 0;
  unsigned maxIndex = 0;

  double totalData = 0.0;
  double totalIterations = 0.0;

  for (unsigned i=0; i<elapsedNs.size(); ++i) {
    totalData += elapsedNs[i];
    totalIterations += (double)iterations[i];

    if (i==0) {
      ns[0] = ns[1] = elapsedNs[i];
      continue;
    }

    if (elapsedNs[i]<ns[0]) {
      ns[0] = elapsedNs[i];
      minIndex = i;
    } else if (elapsedNs[i]>ns[1]) {
      ns[1] = elapsedNs[i];
      maxIndex = i;
    }
  }

  ns[0] = elapsedNs[minIndex];
  ns[1] = elapsedNs[maxIndex];
  ns[2] = totalData / (double)elapsedNs.size();

  nsPerIter[0] = ns[0] / iterations[minIndex];
  nsPerIter[1] = ns[1] / iterations[maxIndex];
  nsPerIter[2] = totalData / totalIterations;

  ops[0] = (double)1000000000.0 / nsPerIter[0];
  ops[1] = (double)1000000000.0 / nsPerIter[1];
  ops[2] = (double)1000000000.0 / nsPerIter[2];

  // This is a hack: assumes all result sets ran with same # of iterations
  iters[0] = iters[1] = iters[2] = iterations[0];
}

void Intel::Stats::calcMinMaxAvgData(const std::vector<u_int64_t>& data, const std::vector<u_int64_t>& iterations,
  double& min, double& max, double& avg) const {

  assert(data.size()==iterations.size());

  unsigned minIndex = 0;
  unsigned maxIndex = 0;

  double totalData = 0.0;
  double totalIterations = 0.0;

  for (unsigned i=0; i<data.size(); ++i) {
    totalData += (double)data[i];
    totalIterations += (double)iterations[i];

    double datum = (double)data[i];

    if (i==0) {
      min = max = datum;
      continue;
    }

    if (data[i]<min) {
      min = datum;
      minIndex = i;
    } else if (data[i]>max) {
      max = datum;
      maxIndex = i;
    }
  }

  min /= (double)iterations[minIndex];
  max /= (double)iterations[maxIndex];
  avg = totalData / totalIterations;
}

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

  sprintf(buffer, "million of iterations per second");
  printf("%-3s [%-60s]\n", "MPS", buffer);

  sprintf(buffer, "iterations per second");
  printf("%-3s [%-60s]\n", "OPS", buffer);

  sprintf(buffer, "iterations");
  printf("%-3s [%-60s]\n", "N", buffer);
}

void Intel::Stats::dump(const Intel::SkyLake::PMU& pmu) const {
  for (unsigned i=0; i<d_description.size(); ++i) {
    printf("Result Set %d: %s: Intel::Skylake CPU HW core %d\n", i, d_description[i].c_str(), pmu.core());

    printf(  "%-3s: [%-60s] value: %lu\n", "C0", "rdtsc cycles: use with F2", d_rdstc[i]); 

    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[0].c_str(), pmu.fixedDescription()[0].c_str(), d_fixedCntr0[i]);
    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[1].c_str(), pmu.fixedDescription()[1].c_str(), d_fixedCntr1[i]);
    printf(  "%-3s: [%-60s] value: %lu\n", pmu.fixedMnemonic()[2].c_str(), pmu.fixedDescription()[2].c_str(), d_fixedCntr2[i]);

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
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "MPS", "millions of operations per second", (double)1000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "OPS", "operations per second", (double)1000000000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %lu\n",  "N", "iterations", d_itertions[i]);
  }
}

void Intel::Stats::dumpScaled(const Intel::SkyLake::PMU& pmu) const {
  for (unsigned i=0; i<d_description.size(); ++i) {
    printf("Scaled Result Set %d: %s: Intel::Skylake CPU HW core %d\n", i, d_description[i].c_str(), pmu.core());

    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "C0", "rdtsc cycles: use with F2", (double)d_rdstc[i]/(double)d_itertions[i]); 

    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[0].c_str(), pmu.fixedDescription()[0].c_str(), (double)d_fixedCntr0[i]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[1].c_str(), pmu.fixedDescription()[1].c_str(), (double)d_fixedCntr1[1]/(double)d_itertions[i]);
    printf(  "%-3s: [%-60s] value: %-11.5f\n", pmu.fixedMnemonic()[2].c_str(), pmu.fixedDescription()[2].c_str(), (double)d_fixedCntr2[i]/(double)d_itertions[i]);

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
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "MPS", "millions of operations per second", (double)1000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %-11.5lf\n", "OPS", "operations per second", (double)1000000000/((double)d_elapsedNs[i]/(double)d_itertions[i]));
    printf(  "%-3s: [%-60s] value: %lu\n",  "N", "iterations", d_itertions[i]);
  }
}

void Intel::Stats::summary(const char *label, const Intel::SkyLake::PMU& pmu) const {
  printf("Scaled Summary Statistics: %lu runs: %s\n", d_itertions.size(), label);

  double min, max, avg;

  calcMinMaxAvgData(d_rdstc, d_itertions, min, max, avg);
  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n", "C0", "rdtsc cycles: use with F2", min, max, avg);

  calcMinMaxAvgData(d_fixedCntr0, d_itertions, min, max, avg);
  printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
    pmu.fixedMnemonic()[0].c_str(),
    pmu.fixedDescription()[0].c_str(),
    min, max, avg);

  calcMinMaxAvgData(d_fixedCntr1, d_itertions, min, max, avg);
  printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
    pmu.fixedMnemonic()[1].c_str(),
    pmu.fixedDescription()[1].c_str(),
    min, max, avg);

  calcMinMaxAvgData(d_fixedCntr2, d_itertions, min, max, avg);
  printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
    pmu.fixedMnemonic()[2].c_str(),
    pmu.fixedDescription()[2].c_str(),
    min, max, avg);

  if (pmu.programmableCounterDefined()>0) {
    calcMinMaxAvgData(d_progmCntr0, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[0].c_str(),
      pmu.progDescription()[0].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>1) {
    calcMinMaxAvgData(d_progmCntr1, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[1].c_str(),
      pmu.progDescription()[1].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>2) {
    calcMinMaxAvgData(d_progmCntr2, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[2].c_str(),
      pmu.progDescription()[2].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>3) {
    calcMinMaxAvgData(d_progmCntr3, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[3].c_str(),
      pmu.progDescription()[3].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>4) {
    calcMinMaxAvgData(d_progmCntr4, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[4].c_str(),
      pmu.progDescription()[4].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>5) {
    calcMinMaxAvgData(d_progmCntr5, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[5].c_str(),
      pmu.progDescription()[5].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>6) {
    calcMinMaxAvgData(d_progmCntr6, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[6].c_str(),
      pmu.progDescription()[6].c_str(),
      min, max, avg);
  }

  if (pmu.programmableCounterDefined()>7) {
    calcMinMaxAvgData(d_progmCntr7, d_itertions, min, max, avg);
    printf(  "%-3s: [%-60s] minValue: %-16.5f maxValue: %-16.5lf avgValue: %-16.5lf\n",
      pmu.progMnemonic()[7].c_str(),
      pmu.progDescription()[7].c_str(),
      min, max, avg);
  }

  double ns[3];
  double nsPerIter[3];
  double ops[3];
  double iters[3];
  calcMinMaxAvgTime(d_elapsedNs, d_itertions, ns, nsPerIter, ops, iters);

  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n",
    "NSI",
    "nanoseconds per iteration",
    nsPerIter[0], nsPerIter[1], nsPerIter[2]);

  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n",
    "MPS",
    "millions of operations per second",
    ops[0]/1000000.0, ops[1]/1000000.0, ops[2]/1000000.0);


  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n",
    "OPS",
    "operations per second",
    ops[0], ops[1], ops[2]);

  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n",
    "N",
    "iterations",
    iters[0], iters[1], iters[2]);

  printf(  "%-3s: [%-60s] minValue: %-16.5lf maxValue: %-16.5lf avgValue: %-16.5f\n",
    "NS",
    "nanoseconds elapsed",
    ns[0], ns[1], ns[2]);
}

void Intel::Stats::record(
    const char *description,                                                                            
    unsigned long iterations,                                                                           
    timespec start,                                          
    timespec end,                                            
    const Intel::SkyLake::PMU& pmu)
{
  // Avoid divide by zero
  assert(iterations>0);

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
