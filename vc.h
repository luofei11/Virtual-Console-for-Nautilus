#ifndef __NK_VC
#define __NK_VC

enum VC_TYPE {RAW, COOKED};
struct virtual_console;

struct virtual_console *nk_create_vc(enum VC_TYPE new_vc_type); 
int nk_destroy_vc(struct virtual_console *vc);

int nk_bind_vc(struct nk_thread *thread, struct virtual_console * cons, void (*newdatacallback)() );
int nk_release_vc(struct nk_thread *thread);
int nk_switch_to_vc(struct virtual_console *vc);
int nk_get_char();
int nk_get_scancode();
//Keycode nk_get_char();
//Scancode nk_get_scancode();
int nk_put_char(int c);
int nk_puts(char *s); 
int nk_set_attr(int attr);
int nk_display_char(uint8_t c, uint8_t attr, uint8_t x, uint8_t y);
int nk_vc_init();
#endif
