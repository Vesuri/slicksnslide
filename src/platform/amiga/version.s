| AmigaOS version string for Slicks.
|
| AmigaDOS Version and archive-inspection tools find this by scanning the load
| image for the "$VER: " marker; no code references it.  SHF_GNU_RETAIN keeps
| the otherwise unreferenced section alive through --gc-sections.
|
| Keep the date hardcoded so identical source trees produce identical builds.
| tools/check_release.py requires the version to match VERSION.
	.section .rodata.version,"aR"
	.balign 2
	.asciz "$VER: Slicks 0.1 (30.09.2026)"
	.balign 2
	.section .data.whd_storage,"awR"
	.balign 4
	.globl slicks_whd_storage
slicks_whd_storage:
	.ascii "SLKSIO01"
	.word 1,40
	.long 0,0
	.long 0,0,0,0,0
