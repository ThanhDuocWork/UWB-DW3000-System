#pragma once

namespace boot_swap {

enum class PrimaryVersion { A, B, Either };

// Verify only complete images. Forward/Reverse slots can be mixed until boot().
// The caller must validate the physical layout before constructing the adapter.
template <class Engine, class VerifyPrimary>
bool boot_flow(Engine &core, VerifyPrimary verify)
{
    using Status = typename Engine::JournalStatus;
    using Mode = typename Engine::Mode;
    const auto status = core.load_status();
    if (status == Status::Blank) return verify(PrimaryVersion::A);
    if (status != Status::Valid) return false;
    const auto initial = core.mode();
    if (initial == Mode::Ready && !verify(PrimaryVersion::B)) return false;
    if (!core.boot()) return false;
    const bool forward = initial == Mode::Forward || initial == Mode::Ready;
    const bool reverse = initial == Mode::Trial || initial == Mode::Reverse;
    if (core.mode() != (forward ? Mode::Trial : Mode::Idle)) return false;
    return verify(forward ? PrimaryVersion::B :
                  reverse ? PrimaryVersion::A : PrimaryVersion::Either);
}

}  // namespace boot_swap
