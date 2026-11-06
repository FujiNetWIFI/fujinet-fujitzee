; menu.inc -- st_menu, from 0 in the lobby or at the table.
;
;   1 (or 0)  back where you were
;   5         the keys
;   2         leave the table / refetch the tables
;   3         change the name (leaving the table first)
;   4         quit to CONFIG (leaving the table first)

        FIT     160
st_menu:
        RESET
        FCALL   d_cls
        PUTS    00H, s_menu
        PUTS    20H, s_m1
        GETV    V_SEAT
        BZ      mn1
        PUTS    30H, s_m2leave
        BR      mn2
mn1:    PUTS    30H, s_m2ref
mn2:    PUTS    40H, s_m3
        PUTS    50H, s_m4
        PUTS    60H, s_m5
mnk:    FCALL   kwait
        PLO     RC
        BZ      mnback
        XRI     1
        BZ      mnback
        GLO     RC
        XRI     2
        BZ      mnlv
        GLO     RC
        XRI     3
        BZ      mnren
        GLO     RC
        XRI     4
        BZ      mnq
        GLO     RC
        XRI     5
        BNZ     mnk
        FJMP    st_help
mnq:    FCALL   mnleave
        FJMP    mnquit
mnback: GETV    V_SEAT
        BZ      mnlob
        FJMP    st_game
mnlv:   FCALL   mnleave
mnlob:  FJMP    st_lobby
mnren:  FCALL   mnleave
        FJMP    st_edit

; mnleave -- if seated, tell the server we went.
        FIT     16
mnleave:
        GETV    V_SEAT
        BZ      mnl9
        PUTS    90H, s_leaving
        FJMP    nleave
mnl9:   FRET

; mnquit -- hand the console back to CONFIG. fn_config returns only if the
; cart never armed it.
mnquit: FCALL   d_cls
        PUTS    40H, s_toconfig
        FCALL   fn_config
        PUTS    60H, s_cfgfail
        FCALL   kwait
        FJMP    st_lobby

; st_help -- the keys, one screen, any key back to the menu.
        FIT     120
st_help:
        RESET
        FCALL   d_cls
        PUTS    00H, s_h0
        PUTS    10H, s_h1
        PUTS    20H, s_h2
        PUTS    30H, s_h3
        PUTS    40H, s_h4
        PUTS    50H, s_h5
        PUTS    70H, s_h6
        PUTS    80H, s_h7
        FCALL   kwait
        FJMP    st_menu
