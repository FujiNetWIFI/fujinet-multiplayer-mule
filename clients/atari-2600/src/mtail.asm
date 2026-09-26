; mtail.asm -- the fixed half's code: the trampoline, the shared transport,
; the cold stub.
;
; $1800-$1F1F is the cartridge's mailbox; the client owns $1F20-$1FFB (the
; fixed tail) and the vectors. Shared code lives here because it is the one
; region every bank sees at the same address. tools/mktail.py reads the
; addresses back out of this listing into build/tail.inc for the banks.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"

        INCLUDE "mcore.inc"

        END
