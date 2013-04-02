#include "kthermal.h"

static struct delayed_work check_temp_workk;
static struct msm_thermal_data msm_thermal_info;
static unsigned int prev_freq;
static bool isthrottling = false;

static void __cpuinit check_tempk(struct work_struct *work)
{
	static int limit_init;
	struct tsens_device tsens_dev;
	long temp = 0;
	uint32_t max_freq = 1512000;
	int cpu = 0;
	int ret = 0;
	struct cpufreq_policy *policy;
	
	tsens_dev.sensor_num = msm_thermal_info.sensor_id;
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (ret) {
		pr_debug("%s: Unable to read TSENS sensor %d\n",
				KBUILD_MODNAME, tsens_dev.sensor_num);
		goto reschedule;
	}
	pr_alert("CHECK TEMP %lu-%d-%d\n", temp, msm_thermal_info.limit_temp_degC, msm_thermal_info.temp_hysteresis_degC);
	
	if (temp >= msm_thermal_info.limit_temp_degC)
	{
		int i;
		if (!isthrottling)
			prev_freq = cpufreq_get(0);
		isthrottling = true;
		pr_alert("THROTTLING\n");
		policy = cpufreq_cpu_get(0);
		__cpufreq_driver_target(policy, 1296000, CPUFREQ_RELATION_H);
		/*for (i = 0; i < num_online_cpus(); i++)
		{
			if (cpu_online(i))
			{
				policy = cpufreq_cpu_get(i);
				if (policy != NULL)
					cpufreq_driver_target(policy, 1296000, CPUFREQ_RELATION_H);
			}
		}*/
	}
	else if (isthrottling)
	{
		int i;
		isthrottling = false;
		pr_alert("NOT THROTTLING\n");
		policy = cpufreq_cpu_get(0);
		if (prev_freq > 0)
			__cpufreq_driver_target(policy, prev_freq, CPUFREQ_RELATION_H);
		/*for (i = 0; i < num_online_cpus(); i++)
		{
			if (cpu_online(i))
			{
				policy = cpufreq_cpu_get(i);
				if (prev_freq > 0 && policy != NULL)
					__cpufreq_driver_target(policy, prev_freq, CPUFREQ_RELATION_H);
			}
		}*/
	}
	/*if (!limit_init) {
		ret = msm_thermal_get_freq_table();
		if (ret)
			goto reschedule;
		else
			limit_init = 1;
	}

	do_core_control(temp);

	if (temp >= msm_thermal_info.limit_temp_degC) {
		if (limit_idx == limit_idx_low)
			goto reschedule;

		limit_idx -= msm_thermal_info.freq_step;
		if (limit_idx < limit_idx_low)
			limit_idx = limit_idx_low;
		max_freq = table[limit_idx].frequency;
	} else if (temp < msm_thermal_info.limit_temp_degC -
		 msm_thermal_info.temp_hysteresis_degC) {
		if (limit_idx == limit_idx_high)
			goto reschedule;

		limit_idx += msm_thermal_info.freq_step;
		if (limit_idx >= limit_idx_high) {
			limit_idx = limit_idx_high;
			max_freq = MSM_CPUFREQ_NO_LIMIT;
		} else
			max_freq = table[limit_idx].frequency;
	}
	if (max_freq == limited_max_freq)
		goto reschedule;

	// Update new limits
	for_each_possible_cpu(cpu) {
		ret = update_cpu_max_freq(cpu, max_freq);
		if (ret)
			pr_debug(
			"%s: Unable to limit cpu%d max freq to %d\n",
					KBUILD_MODNAME, cpu, max_freq);
	}*/

reschedule:
	schedule_delayed_work_on(0, &check_temp_workk,
			msecs_to_jiffies(msm_thermal_info.poll_ms));
}

static int __init start_kthermal(void)
{
	pr_alert("START KTHERMAL\n");
	msm_thermal_info = msm_thermal_pdata;
	INIT_DELAYED_WORK(&check_temp_workk, check_tempk);
	schedule_delayed_work_on(0, &check_temp_workk, msecs_to_jiffies(60000));
	
	return 0;
}
late_initcall(start_kthermal);
