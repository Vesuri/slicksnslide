#ifndef SLICKS_AMIGA_SHOP_H
#define SLICKS_AMIGA_SHOP_H
#include "amiga_player_menu.h"
#include "../../ui/shop_draw.h"
int slicks_amiga_shop_check_create_failures(struct SlicksResourceArchive *,
    unsigned char *,const struct SlicksShopContent *);
/* Dedicated surface; decoded original background is kept in saved[]. */
struct SlicksAmigaPlayerMenu *slicks_amiga_shop_create(struct SlicksResourceArchive *,
    unsigned char *,const struct SlicksShopContent *,struct SlicksShopMenu *);
int slicks_amiga_shop_draw(struct SlicksAmigaPlayerMenu *,const struct SlicksShopContent *,
    const struct SlicksShopMenu *);
int slicks_amiga_shop_refresh(struct SlicksAmigaPlayerMenu *,const struct SlicksShopContent *,
    const struct SlicksShopMenu *,signed char,signed char);
#endif
