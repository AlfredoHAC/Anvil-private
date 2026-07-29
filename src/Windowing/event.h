#ifndef ANVL_EVENT_HEADER
#define ANVL_EVENT_HEADER

#include "Core/typedefs.h"

typedef enum EventType
{
    ANVL_EVENT_TYPE_NONE = 0,
    ANVL_EVENT_TYPE_WINDOW_CLOSE,
    ANVL_EVENT_TYPE_WINDOW_RESIZE,
    ANVL_EVENT_TYPE_KEY_PRESS,
    ANVL_EVENT_TYPE_KEY_RELEASE,
    ANVL_EVENT_TYPE_MOUSE_MOVE,
    ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
    ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
    ANVL_EVENT_TYPE_MOUSE_SCROLL,
} EventType;

typedef struct WindowCloseEvent
{
    char _pad[1];
} WindowCloseEvent;

typedef struct WindowResizeEvent
{
    uint16 width;
    uint16 height;
} WindowResizeEvent;

typedef struct KeyPressEvent
{
    uint16 key_code;
    uint8  modifier_set;
} KeyPressEvent;

typedef struct KeyReleaseEvent
{
    uint16 key_code;
    uint8  modifier_set;
} KeyReleaseEvent;

typedef struct MouseMoveEvent
{
    float32 x;
    float32 y;
    uint8   modifier_set;
} MouseMoveEvent;

typedef struct MouseButtonClickEvent
{
    float32 x;
    float32 y;
    uint8   button_code;
    uint8   modifier_set;
} MouseButtonClickEvent;

typedef struct MouseButtonReleaseEvent
{
    float32 x;
    float32 y;
    uint8   button_code;
    uint8   modifier_set;
} MouseButtonReleaseEvent;

typedef struct MouseScrollEvent
{
    float32 x_offset;
    float32 y_offset;
} MouseScrollEvent;

typedef struct Event
{
    EventType type;
    bool      handled;

    union
    {
        WindowCloseEvent        window_close;
        WindowResizeEvent       window_resize;
        KeyPressEvent           key_press;
        KeyReleaseEvent         key_release;
        MouseMoveEvent          mouse_move;
        MouseButtonClickEvent   mouse_button_click;
        MouseButtonReleaseEvent mouse_button_release;
        MouseScrollEvent        mouse_scroll;
    };
} Event;

#endif // !ANVL_EVENT_HEADER
