#include <stdio.h>
#include <stdlib.h>
#include "../src/game/configuration.h"
#include "../src/ui/options_menu.h"

/* Build-time only: emit typed data, never a guest-memory image or CPU state.
 * Both input and generated output are local-only original-derived assets. */
static void bytes(FILE *out,const char *name,const unsigned char *values,unsigned count)
{
    fprintf(out,"    .%s = {",name);
    for(unsigned i=0;i<count;++i) fprintf(out,"%s%u",i?",":"",values[i]);
    fputs("},\n",out);
}
static void words(FILE *out,const char *name,const short *values,unsigned count)
{
    fprintf(out,"    .%s = {",name);
    for(unsigned i=0;i<count;++i) fprintf(out,"%s%d",i?",":"",values[i]);
    fputs("},\n",out);
}
static int strings(FILE *out,const unsigned char *data,unsigned size,
    const char *name,unsigned table,unsigned count)
{
    fprintf(out,"static const unsigned char %s[%u][64] = {\n",name,count);
    for(unsigned i=0;i<count;++i) {
        unsigned at=table+4*i;
        if(at+4>size) return -1;
        unsigned long address=(unsigned short)slicks_config_default_word(data,at)+
            16UL*(unsigned short)slicks_config_default_word(data,at+2);
        /* A null entry is a runtime-supplied label (Tracks random-order). */
        if(!address) { fputs("    {0},\n",out); continue; }
        if(address<0x3cbf0UL || address-0x3cbf0UL>=size) return -1;
        unsigned offset=(unsigned)(address-0x3cbf0UL),length=0;
        while(offset+length<size && length<64 && data[offset+length]) ++length;
        if(length==64 || offset+length>=size) return -1;
        fputs("    {",out);
        for(unsigned j=0;j<=length;++j) fprintf(out,"%s%u",j?",":"",data[offset+j]);
        fputs("},\n",out);
    }
    fputs("};\n",out); return 0;
}
int main(int argc,char **argv)
{
    if(argc!=3) { fprintf(stderr,"usage: %s runtime.bin setup_defaults.h\n",argv[0]); return 2; }
    FILE *in=fopen(argv[1],"rb");
    if(!in) { perror(argv[1]); return 2; }
    unsigned char data[0x2fa4];
    if(fseek(in,0x3cbf0-0x10100,SEEK_SET) || fread(data,1,sizeof data,in)!=sizeof data) {
        fprintf(stderr,"Missing initialized DS data in %s\n",argv[1]); fclose(in); return 2;
    }
    fclose(in);
    struct SlicksConfiguration config;
    if(slicks_configuration_defaults(&config,data,sizeof data)) return 2;
    FILE *out=fopen(argv[2],"w");
    if(!out) { perror(argv[2]); return 2; }
    fputs("/* Generated from supplied original executable; LOCAL ONLY, DO NOT COMMIT. */\n"
          "#ifndef SLICKS_GENERATED_SETUP_DEFAULTS_H\n#define SLICKS_GENERATED_SETUP_DEFAULTS_H\n"
          "#include \"../game/configuration.h\"\n"
          "static const struct SlicksConfiguration slicks_original_configuration = {\n",out);
#define FIELD(name) fprintf(out,"    ." #name " = %d,\n",config.name)
    FIELD(field_05e1); FIELD(field_062e); FIELD(date_code);
    FIELD(field_05de); FIELD(field_0626);
    FIELD(field_172c); FIELD(field_1728); FIELD(field_719c);
    FIELD(field_172e); FIELD(field_172a); FIELD(field_719e);
#undef FIELD
    bytes(out,"keys",config.keys,20);
    bytes(out,"player_input",config.player_input,4);
    bytes(out,"bindings",config.bindings,60);
    words(out,"options",config.options,15);
    words(out,"selected_profile",config.selected_profile,4);
    fputs("};\nstatic const unsigned char slicks_original_mode_flags[6] = {",out);
    for(unsigned i=0;i<6;++i) fprintf(out,"%s%u",i?",":"",data[0x1157+i]);
    fputs("};\nstatic const unsigned char slicks_original_finish_points[4] = {",out);
    for(unsigned i=0;i<4;++i) fprintf(out,"%s%u",i?",":"",data[0x454+i]);
    fprintf(out,"};\nstatic const unsigned char slicks_original_fastest_points = %u;\n",data[0x458]);
    fputs("static const unsigned char slicks_original_profile_colours[100][6] = {\n",out);
    for(unsigned i=0;i<100;++i) {
        fputs("    {",out);
        for(unsigned j=0;j<6;++j) fprintf(out,"%s%u",j?",":"",data[0x1db+6*i+j]);
        fputs("},\n",out);
    }
    fputs("};\nstatic const char slicks_original_profile_labels[3][21] = {\n",out);
    const unsigned offsets[]={0x143a,0x1441,0x144a};
    for(unsigned i=0;i<3;++i) {
        unsigned length=0;
        while(length<21 && data[offsets[i]+length]) ++length;
        if(length==21) { fclose(out); return 2; }
        fputs("    {",out);
        for(unsigned j=0;j<=length;++j) fprintf(out,"%s%u",j?",":"",data[offsets[i]+j]);
        fputs("},\n",out);
    }
    fputs("};\nstatic const unsigned char slicks_original_fallback_colours[4][6] = {\n",out);
    for(unsigned i=0;i<4;++i) {
        fputs("    {",out);
        for(unsigned j=0;j<6;++j) fprintf(out,"%s%u",j?",":"",data[0x433+6*i+j]);
        fputs("},\n",out);
    }
    fprintf(out,"};\nstatic const short slicks_original_override_count = %d;\n",
        slicks_config_default_word(data,0xf1a));
    fputs("static const unsigned char slicks_original_vehicle_weights[10] = {",out);
    for(unsigned i=0;i<10;++i) fprintf(out,"%s%u",i?",":"",data[0x1b4+i]);
    fputs("};\nstatic const unsigned char slicks_original_item_flags[13] = {",out);
    for(unsigned i=0;i<13;++i) fprintf(out,"%s%u",i?",":"",data[0x106f+i]);
    fputs("};\nstatic const signed char slicks_original_item_capacity[13] = {",out);
    for(unsigned i=0;i<13;++i) fprintf(out,"%s%d",i?",":"",(signed char)data[0x10a4+i]);
    fputs("};\n",out);
    fputs("#include \"../game/weapon_shop.h\"\nstatic const struct SlicksShopRules slicks_original_shop_rules = {\n",out);
    bytes(out,"flags",data+0x106f,13);
    const unsigned shop_at[]={0x10a4,0x107c,0x1aa,0x13e};
    const unsigned shop_count[]={13,13,10,8};
    const char *shop_names[]={"capacity","batch","vehicle_capacity","weapon_weight"};
    for(unsigned i=0;i<4;++i) {
        fprintf(out,"    .%s = {",shop_names[i]);
        for(unsigned j=0;j<shop_count[i];++j) fprintf(out,"%s%d",j?",":"",(signed char)data[shop_at[i]+j]);
        fputs("},\n",out);
    }
    short prices[13];
    for(unsigned j=0;j<13;++j) prices[j]=slicks_config_default_word(data,0x108a+2*j);
    words(out,"base_price",prices,13);
    for(unsigned j=0;j<13;++j) prices[j]=slicks_config_default_word(data,0x17c+2*j);
    words(out,"ammunition_price",prices,13);
    fprintf(out,"};\nstatic const unsigned char slicks_original_shop_extra = %u;\n",data[0x62f]);
    fputs("static const unsigned char slicks_original_shop_items[13][15] = {\n",out);
    for(unsigned i=0;i<13;++i) {
        fputs("    {",out);
        for(unsigned j=0;j<15;++j) fprintf(out,"%s%u",j?",":"",data[0xfac+15*i+j]);
        fputs("},\n",out);
    }
    fputs("};\n",out);
    const unsigned shop_strings[]={0x1531,0x1575,0x1523,0x157f};
    const char *shop_string_names[]={"footer","register","exit","help"};
    for(unsigned i=0;i<4;++i) {
        fprintf(out,"static const unsigned char slicks_original_shop_%s[] = {",shop_string_names[i]);
        unsigned at=shop_strings[i];
        do { fprintf(out,"%s%u",at==shop_strings[i]?"":",",data[at]); } while(data[at++]);
        fputs("};\n",out);
    }
    fputs("#include \"../game/weapon_rules.h\"\nstatic const struct SlicksWeaponRules slicks_original_weapon_rules = {\n",out);
    const unsigned weapon_words[]={0x10e,0x126,0x156};
    const char *weapon_word_names[]={"delay","damage","lifetime"};
    for(unsigned i=0;i<3;++i) {
        for(unsigned j=0;j<8;++j) prices[j]=slicks_config_default_word(data,weapon_words[i]+2*j);
        words(out,weapon_word_names[i],prices,8);
    }
    fprintf(out,"    .unlimited = %d,\n",slicks_config_default_word(data,0x1a8));
    const unsigned weapon_at[]={0x11e,0x136,0x146,0x14e,0x165,0x16e,0x176,0x17e};
    const char *weapon_names[]={"radius","force","shots","spread","ranges","speed","effect","muzzle"};
    for(unsigned i=0;i<8;++i) {
        fprintf(out,"    .%s = {",weapon_names[i]);
        for(unsigned j=0;j<(i==4?9U:8U);++j) fprintf(out,"%s%d",j?",":"",(signed char)data[weapon_at[i]+j]);
        fputs("},\n",out);
    }
    bytes(out,"fire_sound",data+0x196,8);bytes(out,"hit_sound",data+0x19e,8);
    fputs("};\n",out);
    const unsigned menu_offsets[]={0x1365,0x136d,0x1393,0x1395,0x1110,0x111b,0x1126,0x1131,0x1343,0x13a0,0x13b5};
    const char *menu_names[]={"title","footer","random","random_each","add","edit","delete","exit","select","actions","delete_question"};
    for(unsigned i=0;i<sizeof menu_offsets/sizeof menu_offsets[0];++i) {
        unsigned length=0; while(length<64 && data[menu_offsets[i]+length]) ++length;
        if(length==64) { fclose(out); return 2; }
        fprintf(out,"static const unsigned char slicks_original_players_%s[] = {",menu_names[i]);
        for(unsigned j=0;j<=length;++j) fprintf(out,"%s%u",j?",":"",data[menu_offsets[i]+j]);
        fputs("};\n",out);
    }
    fprintf(out,"static const unsigned char slicks_original_players_footer_percent = %u;\n",data[0x16fd]);
    const unsigned editor_offsets[]={0x10ce,0x10fe,0x1280,0x1300,0x1314,0x132a};
    const unsigned editor_sizes[]={48,18,2,20,22,14};
    const char *editor_names[]={"rows","roles","percent","random","random_each","unavailable"};
    for(unsigned i=0;i<6;++i) {
        fprintf(out,"static const unsigned char slicks_original_editor_%s[] = {",editor_names[i]);
        for(unsigned j=0;j<editor_sizes[i];++j) fprintf(out,"%s%u",j?",":"",data[editor_offsets[i]+j]);
        fputs("};\n",out);
    }
    fprintf(out,"static const unsigned char slicks_original_editor_field_01a6 = %u;\n",data[0x1a6]);
    fputs("static const unsigned char slicks_original_editor_name_caption[] = {",out);
    for(unsigned j=0;j<5;++j) fprintf(out,"%s%u",j?",":"",data[0x1338+j]);
    fputs("};\n",out);
    fputs("static const unsigned char slicks_original_editor_colour_caption[] = {",out);
    for(unsigned j=0;j<6;++j) fprintf(out,"%s%u",j?",":"",data[0x133d+j]);
    fputs("};\n",out);
    struct SlicksOptionSpec specs[15];
    if(slicks_option_specs(specs,data,sizeof data)) { fclose(out); return 2; }
    fputs("#include \"../ui/options_menu.h\"\n"
        "static const struct SlicksOptionSpec slicks_original_option_specs[15] = {\n",out);
    for(unsigned i=0;i<15;++i) fprintf(out,"    {%d,%d,%d,%u},\n",
        specs[i].maximum,specs[i].minimum,specs[i].step,specs[i].modes);
    fputs("};\n",out);
    if(strings(out,data,sizeof data,"slicks_original_option_labels",0xf1c,18) ||
       strings(out,data,sizeof data,"slicks_original_option_suffixes",0xf64,15) ||
       strings(out,data,sizeof data,"slicks_original_mode_labels",0x7e2,6) ||
       strings(out,data,sizeof data,"slicks_original_race_menu_keys",0x79a,6) ||
       strings(out,data,sizeof data,"slicks_original_intermission_keys",0x7c2,4)) {
        fclose(out); return 2;
    }
    const unsigned save_offsets[]={0x783,0xb6f,0xb7b,0xb8e,0xbb0,0xbbb};
    const char *save_names[]={"save","load","empty","delete","deleted","name"};
    for(unsigned i=0;i<6;++i) {
        fprintf(out,"static const unsigned char slicks_original_saved_%s[] = {",save_names[i]);
        unsigned at=save_offsets[i];
        do { fprintf(out,"%s%u",at==save_offsets[i]?"":",",data[at]); } while(data[at++]);
        fputs("};\n",out);
    }
    fputs("static const unsigned char slicks_original_change_cars_title[] = {",out);
    for(unsigned i=0;i<5;++i) fprintf(out,"%s%u",i?",":"",data[0xc3d+i]);
    fputs("};\n",out);
    fputs("static const unsigned char slicks_original_options_title[] = {",out);
    for(unsigned i=0;i<9;++i) fprintf(out,"%s%u",i?",":"",data[0x13ca+i]);
    fputs("};\nstatic const unsigned char slicks_original_options_boolean[2][4] = {",out);
    for(unsigned i=0;i<2;++i) {
        fprintf(out,"%s{",i?",":"");
        for(unsigned j=0;j<4;++j) fprintf(out,"%s%u",j?",":"",data[0x113c+4*i+j]);
        fputs("}",out);
    }
    fputs("};\n",out);
    fputs("static const unsigned char slicks_original_controller_scans[69] = {",out);
    for(unsigned i=0;i<69;++i) fprintf(out,"%s%u",i?",":"",data[0x45a+i]);
    fputs("};\n",out);
    if(strings(out,data,sizeof data,"slicks_original_controller_key_names",0x4a0,69)) {
        fclose(out); return 2;
    }
    const unsigned controller_offsets[]={0x15e7,0x1523,0x15f0};
    const unsigned controller_sizes[]={9,3,4};
    const char *controller_names[]={"defaults","exit","prompt"};
    for(unsigned i=0;i<3;++i) {
        fprintf(out,"static const unsigned char slicks_original_controller_%s[] = {",controller_names[i]);
        for(unsigned j=0;j<controller_sizes[i];++j)
            fprintf(out,"%s%u",j?",":"",data[controller_offsets[i]+j]);
        fputs("};\n",out);
    }
    if(strings(out,data,sizeof data,"slicks_original_track_actions",0x10b6,6)) {
        fclose(out); return 2;
    }
    const unsigned track_offsets[]={0x1296,0x129d,0x12e0,0x12eb,0x1307,0x1309};
    const char *track_names[]={"title","footer","random_on","random_off","separator","help"};
    for(unsigned i=0;i<6;++i) {
        fprintf(out,"static const unsigned char slicks_original_track_%s[] = {",track_names[i]);
        unsigned at=track_offsets[i];
        do { fprintf(out,"%s%u",at==track_offsets[i]?"":",",data[at]); } while(data[at++]);
        fputs("};\n",out);
    }
    fprintf(out,"static const unsigned char slicks_original_track_random_order = %u;\n",data[0x624]);
    fprintf(out,"static const unsigned char slicks_original_date_separator = %u;\n",data[0x171e]);
    fprintf(out,"static const signed char slicks_original_date_order = %d;\n",(signed char)data[0x171f]);
    const unsigned list_offsets[]={0x15a5,0x15bb,0x15ca};
    const char *list_names[]={"actions","name","delete_question"};
    for(unsigned i=0;i<3;++i) {
        fprintf(out,"static const unsigned char slicks_original_track_list_%s[] = {",list_names[i]);
        unsigned at=list_offsets[i];
        do { fprintf(out,"%s%u",at==list_offsets[i]?"":",",data[at]); } while(data[at++]);
        fputs("};\n",out);
    }
    const unsigned clear_offsets[]={0x951,0x974};
    const char *clear_names[]={"question","complete"};
    for(unsigned i=0;i<2;++i) {
        fprintf(out,"static const unsigned char slicks_original_clear_%s[] = {",clear_names[i]);
        unsigned at=clear_offsets[i];
        do { fprintf(out,"%s%u",at==clear_offsets[i]?"":",",data[at]); } while(data[at++]);
        fputs("};\n",out);
    }
    fprintf(out,"static const signed char slicks_original_device_interval = %d;\n",(signed char)data[0x6ec]);
    fputs("#endif\n",out);
    int failed=ferror(out);
    if(fclose(out)) failed=1;
    return failed?2:0;
}
