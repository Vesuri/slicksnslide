#ifndef SLICKS_TRACK_SCENE_H
#define SLICKS_TRACK_SCENE_H

/*
 * Build the first race background from the original SLICKS.DAT image stream
 * and a version-2 .SS track.  The destination is the native four-bank VGA
 * store used by the translated graphics primitives.
 */
int slicks_build_track_scene(unsigned char *logical,
                             const unsigned char *dat,
                             unsigned long dat_size,
                             const unsigned char *track,
                             unsigned long track_size,
                             unsigned char *sprite_arena,
                             unsigned long arena_size);

#endif
