#ifndef SLICKS_AMIGA_SHOP_H
#define SLICKS_AMIGA_SHOP_H
#include "amiga_player_menu.h"
#include "../../ui/shop_menu.h"
#include "../../game/setup_session.h"
struct SlicksShopContent {
    struct SlicksSetupSession *session;
    const struct SlicksShopRules *rules;
    const unsigned char (*items)[15];
    const unsigned char *names[4],*footer,*exit_label,*register_label;
    unsigned char extra;
    short track,total;
};
/* Dedicated surface; decoded original background is kept in saved[]. */
struct SlicksAmigaPlayerMenu *slicks_amiga_shop_create(struct SlicksResourceArchive *,
    unsigned char *,const struct SlicksShopContent *,struct SlicksShopMenu *);
int slicks_amiga_shop_draw(struct SlicksAmigaPlayerMenu *,const struct SlicksShopContent *,
    const struct SlicksShopMenu *);
#endif
