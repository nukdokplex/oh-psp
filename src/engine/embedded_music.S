.section .rodata

.globl track1_mp3_start
.globl track1_mp3_end
.balign 4
track1_mp3_start:
.incbin "assets/music/track1_title.mp3"
track1_mp3_end:

.globl track2_mp3_start
.globl track2_mp3_end
.balign 4
track2_mp3_start:
.incbin "assets/music/track2_ingame1.mp3"
track2_mp3_end:

.globl track3_mp3_start
.globl track3_mp3_end
.balign 4
track3_mp3_start:
.incbin "assets/music/track3_ingame2.mp3"
track3_mp3_end:

#if defined(__ELF__) && defined(__linux__)
.section .note.GNU-stack,"",@progbits
#endif
