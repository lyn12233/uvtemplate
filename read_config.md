## FreeRTOSConfig.h behaviors

### common configs
|macro|exapmle value|description|
|---|---|---|
|minimal_stack_size|128|.|
|max_priorities|||
|use_16_bit_ticks|||
|kernel_interrupt_priority|||
|max_syscall_interrupt_priority|||
|max_api_call_interrupt_priority|||
|assert|||
|enable_fpu|||
|enable_mve||m-profile vector extension|

### allocation and deallocation
|macro|exapmle value|description|
|---|---|---|
|total_heap_size|8192|.|
|support_static_allocation|||
|support_dynamic_allocation|||
|application_allocated_heap||manually specify heap inst|
|stack_allocation_from_separate_heap|||
|heap_clear_memory_on_free|||


### tick frequencies
|macro|exapmle value|description|
|---|---|---|
|tick_rate_hz|1000|tick rate of |
|cpu_clock_hz|||
|systick_clock_hz||cortex_m only|

### scheduler configs
|macro|exapmle value|description|
|---|---|---|
|use_preemption|1|.|
|use_port_optimized_task_selection|0|?|
|use_tickless_idle|0|low power mode tickless|
|use_time_slicing|1||
|idle_task_should_yield||idle-level task's unblocking overrides idle-level tasks|

### hooks
|macro|exapmle value|description|
|---|---|---|
|use_idle_hook|||
|use_malloc_filed_hook|||
|use_daemon_startup_hook|||
|use_sb_completed_callback||streambuffer completion callback|
|use_tick_hook|||
|check_for_stack_overflow|||
|minmal_stack_size|128|.|
|stack_depth_type|||

### task and list util
|macro|exapmle value|description|
|---|---|---|
|num_thread_local_storage_pointers|||
|use_minilist_item|||
|max_task_name_len|||

### queue util
|macro|exapmle value|description|
|---|---|---|
|use_mutexes|||
|use_recursive_mutexes|||
|use_counting_semaphores|||
|use_alternative_api||deprecated|
|queue_registry_size||for kernel-aware debugger|
|use_queue_sets|||

### notification util
|macro|exapmle value|description|
|---|---|---|
|use_task_notifications|||
|task_notification_array_entries|||

### timer util
|macro|exapmle value|description|
|---|---|---|
|use_timers|||
|timer_task_priority|||
|timer_queue_length|||

### misc
|macro|exapmle value|description|
|---|---|---|
|use_newlib_reentrant||?|
|enable_backward_compatibility|||
|message_buffer_length_type|||
|use_trace_facility||task visualization and tracing|
|use_stats_formatting_functions||to support xTask_xxx funcs|
|generate_runtime_stats|||
|use_co_routines|||
|max_co_routine_priorities|||
|include_application_define_priviledge_functions||for MPU|
|total_mpu_regions||mpu?|
|tex_s_c_b_flash||mpu|
|tex_s_c_b_sram||mpu|
|enforce_system_calls_from_kernel_only|||
|allow_unpriviledged_critical_sections|||
|enable_trustzone|||
|run_freertos_secure_only|||
|enable_mpu|||