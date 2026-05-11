SESSION_REPORT_IR v1
project = "Lirah-1"
date = "2026-05-08"
status = "paused_for_day"

OBJECTIVE {
  target_behavior = "match NTS-1 mk1 audible behavior on mkII runtime"
  method = "bridge-and-range auditing between header.c, unit.cc, legacy lyre DSP"
}

ACTIONS_COMPLETED {
  A1: verified and kept rollback of broad scaling workaround in bridge
  A2: retained required compile fix by adding Osc.feedback field in legacy header
  A3: retained LFO continuity state persistence patch in legacy cycle path
  A4: cleaned generated build artifacts and metadata noise from git working set
  A5: rebuilt from clean state and confirmed successful make install
  A6: analyzed parameter-domain mismatch (0..100 UI vs legacy 0..1023 normalization paths)
}

CURRENT_WORKTREE {
  modified: "src/legacy/Lyre.cc"
  untracked: "src/legacy/Lyre.hpp"
}

KEY_FINDINGS {
  F1: not all params should be globally scaled to 0..1023
  F2: only legacy destinations using param_val_to_f32 require 10-bit-equivalent input
  F3: cleanest behavior-preserving option is selective scaling in unit.cc forwarding block
  F4: alternative is changing selected header ranges to 1023 plus optional string-display formatting
}

PROPOSED_NEXT_STEP {
  N1: in unit_set_param_value forward-path, scale only mapped_index in {2,5} from 0..100 -> 0..1023
  N2: keep header UI ranges at 0..100 unless user prefers direct 0..1023 exposure
  N3: hardware A/B test knob feel vs mk1 after patch
}

END_STATE {
  user_decision = "done for today"
  pending_code_change = "selective forwarding scale not yet applied"
}
