#include "key.hpp"
#include "car.hpp"
#include "layout.hpp"
#include "timer.hpp"

std::vector<Button> iMagCar::Keypad;

/** @brief 按键初始化
 *  @param[in] pin 用逐飞的定义
 *  @note   是否采用中断都在这里实现
 */
void Button::Init() {
#ifdef BUTTON_MODE_INT
    gpio_interrupt_init(Pin, FALLING, GPIO_INT_CONFIG);
    NVIC_SetPriority(GPIO2_Combined_16_31_IRQn, BUTTON_PRIORITY); // TODO: irqn
#else
    gpio_init(pin, GPI, 0, GPIO_PIN_CONFIG);
#endif
    CAR_ERROR_CHECK(event);
}

/** @brief 查询按键状态
 *  @retval true if pressed
 */
bool Button::IsPressed() { return !gpio_get(pin); }

/**
 * @brief 按钮状态机
 */
void Button::evt_handle() {
    bool value(gpio_get(pin));

    switch (state) {
    case ButtonState::idle:
        if (!value)
            state = ButtonState::press_armed;
        break;

    case ButtonState::press_armed:
        state = value ? ButtonState::idle : ButtonState::press_detected;
        //    DEBUG_LOG("%d\n", state);
        break;

    case ButtonState::press_detected:
        if (value) {
            state = ButtonState::press_armed;
        } else {
            state = ButtonState::pressed;
            event(key_action::key_down);
        }
        break;

    case ButtonState::pressed:
        if (value) {
            state = ButtonState::release_detected;
            pressed_cnt = 0;
        } else {
            ++pressed_cnt;
            if (pressed_cnt >= LONG_PUSH_THRESHOLD / KEY_SCAN_PERIOD) {
                state = ButtonState::long_push;
                pressed_cnt = 0;
            }
        }
        break;

    case ButtonState::long_push:
        if (value)
            state = ButtonState::release_detected;
        else {
            state = ButtonState::idle;
            event(key_action::key_longPush);
        }
        break;

    case ButtonState::release_detected:
        if (value) {
            state = ButtonState::click;
            event(key_action::key_up);
        } else
            state = ButtonState::pressed;
        break;

    case ButtonState::click:
        if (value) {
            ++click_cnt;
            if (click_cnt >= DBL_CLK_THRESHOLD / KEY_SCAN_PERIOD) {
                click_cnt = 0;
                state = ButtonState::idle;
            }
        } else {
            click_cnt = 0;
            state = ButtonState::idle;
            event(key_action::key_doubleClick);
        }
        break;

    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////

static void btnUpHandler(key_action sta) {
    DEBUG_LOG("up %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Up);
    if (sta == key_action::key_longPush) {
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Up);
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Up);
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Up);
    }
}

static void btnDownHandler(key_action sta) {
    DEBUG_LOG("down %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Down);
    if (sta == key_action::key_longPush) {
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Down);
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Down);
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Down);
    }
}

static void btnLeftHandler(key_action sta) {
    DEBUG_LOG("left %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Left);
}

static void btnRightHandler(key_action sta) {
    DEBUG_LOG("right %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Right);
}

static void btnEnterHandler(key_action sta) {
    DEBUG_LOG("enter %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->OnKeyEvent(LayoutBase::KeyEvts::Enter);
    if (sta == key_action::key_longPush) {
        ActiveLayout->GoBack();
        ActiveLayout->GoBack();
    }
}

static void btnEscapeHandler(key_action sta) {
    DEBUG_LOG("esc %d\n", sta);
    if (sta == key_action::key_down)
        ActiveLayout->GoBack();
}

void key_timer_schedule(sched_event_data_t dat) {
    for (auto &k : Car.Keypad) // &&&&&&&&&&&&&&&
        k.evt_handle();
}

static SoftTimer key_timer(key_timer_schedule);

void key_init() {
#define KEYPAD_ASSIGN_EVT(id, evt) Car.Keypad.emplace_back(ButtonList[id], evt)
    KEYPAD_ASSIGN_EVT(BUTTON_UP_ID, btnUpHandler);
    KEYPAD_ASSIGN_EVT(BUTTON_DOWN_ID, btnDownHandler);
    KEYPAD_ASSIGN_EVT(BUTTON_LEFT_ID, btnLeftHandler);
    KEYPAD_ASSIGN_EVT(BUTTON_RIGHT_ID, btnRightHandler);
    KEYPAD_ASSIGN_EVT(BUTTON_ENTER_ID, btnEnterHandler);
    KEYPAD_ASSIGN_EVT(BUTTON_ESCAPE_ID, btnEscapeHandler);
    for (auto k : Car.Keypad)
        k.Init();
    key_timer.Start(KEY_SCAN_PERIOD);
}
