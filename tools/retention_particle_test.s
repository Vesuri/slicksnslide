slicks_retention equ $190000
slicks_race_disable_retention equ $191000
slicks_retention_release equ $192000
slicks_retention_weapons_live equ $192004
slicks_retention_rebuild equ $192008
slicks_retention_touch equ $19200c
slicks_retention_cars equ $192010
slicks_retention_late equ $192014
	include "src/game/sprite_retention.s"
	dc.l .point_y
	dc.l .point_x
	dc.l .point_next
	dc.l .point
