// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct timespec counter_t;

#define GET_COUNTER(P) clock_gettime(CLOCK_THREAD_CPUTIME_ID, P)

#define TICKS(TV) (TV.tv_sec * 1000000000 + TV.tv_nsec)

void update_mean_sigma(long long arg_nsamples,
                       long long arg_sample,
                       long long *arg_maximum,
                       double *arg_mean,
                       double *arg_squared_deviations)
{
  double previous_mean = *arg_mean;
  double previous_squared_deviations = *arg_squared_deviations;

  if (arg_maximum != NULL)
  {
    if (arg_sample > *arg_maximum)
      *arg_maximum = arg_sample;
  }

  *arg_mean = previous_mean + (arg_sample - previous_mean) / arg_nsamples;

  *arg_squared_deviations =
    previous_squared_deviations +
    (arg_sample - previous_mean) * (arg_sample - *arg_mean);
}

#define NCALL 1000000LL

int main(int argc, char **argv)
{
  counter_t counter_dummy;
  counter_t *counter_pointer;

  counter_pointer = &counter_dummy;

  for (int itrial = 0; itrial < 10; itrial++)
  {
    double sample_mean = 0.0;
    double squared_deviations = 0.0;

    for (long long isample = 1; isample <= NCALL; ++isample)
    {
      counter_t counter_stamp;

      GET_COUNTER(&counter_stamp);
      GET_COUNTER(counter_pointer);

      update_mean_sigma(isample,
                        TICKS(counter_dummy) - TICKS(counter_stamp),
                        NULL,
                        &sample_mean,
                        &squared_deviations);
    }

    // note that the distribution is very skewed, so the distribution
    // is not normal:
    // deviations LESS than the mean (or the mean - sigma) hardly ever occur
    // but deviations LARGER than (mean + sigma) do occur more often

    // arbitrarily set the standard deviation to one-third of the mean
    // to check for large positive deviations instead of
    // round(sqrt(sigma / NCALL));

    long long sigma = round(sample_mean / 3.0);
    long long mean = round(sample_mean);

    long long nlarge = 0;
    long long largest = 0;

    for (long long isample = 1; isample <= NCALL; ++isample)
    {
      counter_t counter_stamp;

      GET_COUNTER(&counter_stamp);
      GET_COUNTER(counter_pointer);

      long long delta = TICKS(counter_dummy) - TICKS(counter_stamp);

      if (delta > largest)
        largest = delta;

      if (delta > (mean + 3 * sigma))
        nlarge++;
    }

    printf("NCALL=%lld mean=%lld nlarge=%lld largest=%lld\n",
           NCALL,
           mean,
           nlarge,
           largest);
  }
}
