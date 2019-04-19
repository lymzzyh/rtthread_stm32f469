#include <button.h>

static struct button btn;
struct rt_event button_event;

rt_event_t button_event_handler_get(void)
{
    return &button_event;
}

static uint8_t button_read_pin(void) 
{
    return !rt_pin_read(BUTTON_PIN); 
}

static void button_callback(void *btn)
{    
    rt_event_send(&button_event, BTN_EVENT(get_button_event((struct button *)btn)));
}

static void btn_thread_entry(void* p)
{
    while(1)
    {
        /* 5ms */
        rt_thread_delay(RT_TICK_PER_SECOND/200); 
        button_ticks(); 
    }
}

int multi_button_init(void)
{
    rt_thread_t thread = RT_NULL;

    rt_event_init(&button_event, "btn", RT_IPC_FLAG_FIFO);
    
    /* Create background ticks thread */
    thread = rt_thread_create("btn", btn_thread_entry, RT_NULL, 1024, 10, 10);
    if(thread == RT_NULL)
    {
        return RT_ERROR; 
    }
    rt_thread_startup(thread);

    /* low level drive */
    rt_pin_mode  (BUTTON_PIN, PIN_MODE_INPUT); 
    button_init  (&btn, button_read_pin, PIN_LOW);
    button_attach(&btn, PRESS_DOWN,       button_callback);
    button_attach(&btn, PRESS_UP,         button_callback);
    button_attach(&btn, PRESS_REPEAT,     button_callback);
    button_attach(&btn, SINGLE_CLICK,     button_callback);
    button_attach(&btn, DOUBLE_CLICK,     button_callback);
    button_attach(&btn, LONG_RRESS_START, button_callback);
    button_attach(&btn, LONG_PRESS_HOLD,  button_callback);
    button_start (&btn);

    return RT_EOK; 
}
INIT_ENV_EXPORT(multi_button_init); 
