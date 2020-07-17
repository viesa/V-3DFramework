#include "Keyboard.h"

#include "Event.h"

void Keyboard::Update() noexcept
{
    m_prevKeymap = m_keymap;
    for (auto&[key, state] : m_repeatKeymap)
        state = false;
}

void Keyboard::HandleEvent(const Event &event)
{
    switch (static_cast<Keyboard::Action>(event.keyboard.action))
    {
    case Keyboard::Press:
        OnPress(static_cast<Keyboard::Key>(event.keyboard.key));
        break;
    case Keyboard::Release:
        OnRelease(static_cast<Keyboard::Key>(event.keyboard.key));
        break;
    case Keyboard::Repeat:
        OnRepeat(static_cast<Keyboard::Key>(event.keyboard.key));
        break;
    default: // Keyboard::Action::Unknown not handled
        break;
    }
}

bool Keyboard::IsDown(Key key)
{
    if (m_keymap.find(key) == m_keymap.end())
        m_keymap[key] = false;
    return m_keymap[key];
}

bool Keyboard::WasDown(Keyboard::Key key)
{
    if (m_prevKeymap.find(key) == m_prevKeymap.end())
        m_prevKeymap[key] = false;
    return m_prevKeymap[key];
}

bool Keyboard::IsPressed(Key key)
{
    return IsDown(key) && !WasDown(key);
}

bool Keyboard::IsReleased(Key key)
{
    return !IsDown(key) && WasDown(key);
}

bool Keyboard::IsBeingRepeated(Key key)
{
    if (m_repeatKeymap.find(key) == m_repeatKeymap.end())
        m_repeatKeymap[key] = false;
    return m_repeatKeymap[key];
}

bool Keyboard::IsAnyDown()
{
    for (auto&[key, state] : m_keymap)
    {
        if (state)
            return true;
    }
    return false;
}

void Keyboard::OnPress(Key key) noexcept
{
    m_keymap[key] = true;
}

void Keyboard::OnRelease(Key key) noexcept
{
    m_keymap[key] = false;
}

void Keyboard::OnRepeat(Key key) noexcept
{
    m_repeatKeymap[key] = true;
}
