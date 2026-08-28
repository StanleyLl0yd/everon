#include <type_traits>

#include "HotkeyManager.h"
#include "PowerManager.h"
#include "TrayIcon.h"
#include "Utils.h"

using namespace Everon;

static_assert(!std::is_copy_constructible_v<HotkeyManager>);
static_assert(!std::is_copy_assignable_v<HotkeyManager>);
static_assert(!std::is_move_constructible_v<HotkeyManager>);
static_assert(!std::is_move_assignable_v<HotkeyManager>);

static_assert(!std::is_copy_constructible_v<PowerManager>);
static_assert(!std::is_copy_assignable_v<PowerManager>);
static_assert(!std::is_move_constructible_v<PowerManager>);
static_assert(!std::is_move_assignable_v<PowerManager>);

static_assert(!std::is_copy_constructible_v<TrayIcon>);
static_assert(!std::is_copy_assignable_v<TrayIcon>);
static_assert(!std::is_move_constructible_v<TrayIcon>);
static_assert(!std::is_move_assignable_v<TrayIcon>);

static_assert(!std::is_copy_constructible_v<Utils::SingleInstanceGuard>);
static_assert(!std::is_copy_assignable_v<Utils::SingleInstanceGuard>);
static_assert(!std::is_move_constructible_v<Utils::SingleInstanceGuard>);
static_assert(!std::is_move_assignable_v<Utils::SingleInstanceGuard>);

int main() {
    return 0;
}
