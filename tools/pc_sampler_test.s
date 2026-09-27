g_slicks_pc_sample_frame equ $20000
g_slicks_pc_samples equ $20004
g_slicks_pc_sample_count equ $20008
g_slicks_pc_sample_capacity equ $2000c
g_slicks_pc_sample_missed equ $20010
g_slicks_pc_sample_period equ $20014
g_slicks_pc_sample_timer equ $20018
    include "src/platform/amiga/pc_sampler.s"
