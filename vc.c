#include <nautilus/nautilus.h>
#include <nautilus/irq.h>
#include <nautilus/thread.h>
#include <dev/kbd.h>
#include <nautilus/list.h>
#include <nautilus/cga.h>
#include <nautilus/vc.h>

//#ifndef NAUT_CONFIG_DEBUG_VC
//#undef DEBUG_PRINT
//#define DEBUG_PRINT(fmt, args...) 
//#endif

typedef ushort_t Keycode;
typedef uint8_t Scancode;
//static int g_cur_vc_id;
//static int g_vc_num;

static struct list_head vc_list;
static struct virtual_console *cur_vc = NULL;
static struct virtual_console *default_vc = NULL;


#define Keycode_QUEUE_SIZE 256
#define Scancode_QUEUE_SIZE 512
//#define MAX_VC_NUM 6 //Do we need this? (Switch Console)



struct virtual_console{
    enum VC_TYPE type;
    union queue{
        Scancode s_queue[Scancode_QUEUE_SIZE];
        Keycode k_queue[Keycode_QUEUE_SIZE];
    } keyboard_state;
    uint16_t BUF[80 * 25];
    uint8_t cur_x, cur_y, cur_attr;
    uint16_t head, tail;
    uint32_t num_threads;
    struct list_head waiting_threads_list;
    struct list_head vc_node;
};

void copy_display_to_vc(struct virtual_console *vc) {
  memcpy(vc->BUF, (void*)VGA_BASE_ADDR, sizeof(vc->BUF));
}
void copy_vc_to_display(struct virtual_console *vc) {
  memcpy((void*)VGA_BASE_ADDR, vc->BUF, sizeof(vc->BUF));
}

//struct virtual_console *nk_create_vc("name", RAW | COOKED);//name??
struct virtual_console *nk_create_vc(enum VC_TYPE new_vc_type) {
    struct virtual_console *new_vc = malloc(sizeof(struct virtual_console));
    if(!new_vc) {
        return NULL;
    }
    memset(new_vc, 0, sizeof(struct virtual_console));
    new_vc->type = new_vc_type;
    new_vc->cur_attr = 0xf0;
    new_vc->head = -1;
    new_vc->tail = -1;
    new_vc->num_threads = 0;
    INIT_LIST_HEAD(&new_vc->waiting_threads_list);
    list_add_tail(&new_vc->vc_node, &vc_list);//need to code this function
    return new_vc;
}

int nk_destroy_vc(struct virtual_console *vc) {
    list_del(&vc->vc_node);
    free(vc);
    return 0;
}

// release will destroy vc once the number of binders drops to zero
int nk_bind_vc(nk_thread_t *thread, struct virtual_console * cons, void (*newdatacallback)() ) {
    thread->vc = cons;
    cons->num_threads++;
    return 0;
}
int nk_release_vc(nk_thread_t *thread) {
    thread->vc->num_threads--;
    if(thread->vc->num_threads == 0) {//And it shouldn't be vc 0?
       if(thread->vc != default_vc) {
           nk_destroy_vc(thread->vc);
       }
    }
    thread->vc = NULL;
    return 0;
}

int nk_switch_to_vc(struct virtual_console *vc) {
    copy_display_to_vc(cur_vc);
    cur_vc = vc;
    copy_vc_to_display(cur_vc);
    return 0;
}

// Called needs to know which to use
//int nk_get_char();
//int nk_get_scancode();
//Thread calling these functions? Or theses functions get current threads.
int nk_get_char() {
  // return deque(get_cur_thread()->vc->k_queue);
  return -1;
}
int nk_get_scancode(){
  //return deque(get_cur_thread()->vc->s_queue);
  return -1;
}

// display scrolling or explicitly on screen at a given location
int nk_put_char(int c) {
    struct virtual_console *vc = get_cur_thread()->vc;
    if (!vc) { 
      vc = default_vc;
    }

    if(vc->cur_x >= 80) {
        vc->cur_y++;
        vc->cur_x = 0;
    }
    if(c == '\n') {
        vc->cur_y++;
        vc->cur_x = 0;
    }
    if(vc->cur_y >= 25) {
        //scroll
        vc->cur_y = 0;
    }
    if(c != '\n') {
        nk_display_char(c, vc->cur_attr, vc->cur_x, vc->cur_y);
	vc->cur_x++;
    }
    
    return 0;
}
int nk_puts(char *s) {
    while(*s) {
        nk_put_char(*s);
        s++;
    }
    return 0;
}
int nk_set_attr(int attr) {
    get_cur_thread()->vc->cur_attr = attr;
    return 0;
}
int nk_display_char(uint8_t c, uint8_t attr, uint8_t x, uint8_t y) {
  printk("vc:display %d %d at %d %d\n",c,attr,x,y);
    uint16_t val =(((uint16_t) attr) << 8) + c;
    if(x >= 80 || y >= 25) {
        return -1;
    } else {
        struct virtual_console *thread_vc = get_cur_thread()->vc;
	if (!thread_vc) { 
	  thread_vc = default_vc;
	}
        thread_vc->BUF[y * 25 + x] = val;
        if(thread_vc == cur_vc) {
	  uint16_t *screen = (uint16_t*)VGA_BASE_ADDR;
            screen[y * 25 + x] = val;
        }
    }
    return 0;
}

int nk_vc_init() {
  printk("vc: init\n");
    INIT_LIST_HEAD(&vc_list);
    default_vc = nk_create_vc(RAW);
    if(!default_vc) {
        printk("Error:");
        return -1;
    }
    cur_vc = default_vc;
    copy_display_to_vc(cur_vc);
    return 0;
}

