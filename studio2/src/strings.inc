; strings.inc -- every screen's text. 16 cells a row.

s_title:    DB  "   FUJITZEE", 0
s_nocart:   DB  "NO FUJINET CART", 0
s_chkname:  DB  "CHECKING NAME", 0
s_yourname: DB  "YOUR NAME:", 0
s_saving:   DB  "SAVING NAME", 0
s_edleg1:   DB  "2468 MOVE 5 TYPE", 0
s_edleg2:   DB  "1 DEL     3 DONE", 0
s_choose:   DB  "CHOOSE A TABLE", 0
s_finding:  DB  "FINDING TABLES", 0
s_lbleg:    DB  "1-8 JOIN  0 MENU", 0
s_notables: DB  "NO TABLES", 0
s_neterr:   DB  "NET ERROR ", 0
s_retry:    DB  "0 MENU  5 RETRY", 0
s_menu:     DB  "MENU", 0
s_m1:       DB  "1 BACK", 0
s_m2leave:  DB  "2 LEAVE TABLE", 0
s_m2ref:    DB  "2 REFRESH", 0
s_m3:       DB  "3 CHANGE NAME", 0
s_m4:       DB  "4 QUIT TO CONFIG", 0
s_m5:       DB  "5 HELP", 0
s_leaving:  DB  "LEAVING TABLE", 0
s_toconfig: DB  "TO CONFIG...", 0
s_cfgfail:  DB  "CONFIG FAILED", 0
s_joining:  DB  "JOINING TABLE", 0
s_sending:  DB  "SENDING...", 0
s_hold:     DB  "5 HOLD  1 ROLL ", 0
s_score:    DB  "5 SCORE 1 ROLL ", 0
s_rdyleg:   DB  "5 READY  0 MENU", 0
s_waitleg:  DB  "WAITING TO START", 0
s_gover:    DB  "GAME OVER", 0
s_stand:    DB  "STANDINGS", 0
s_back:     DB  "ANY KEY: BACK", 0

; the help screen, one key per line
s_h0:       DB  "2468 MOVE CURSOR", 0
s_h1:       DB  "5 HOLD / SCORE", 0
s_h2:       DB  "1 ROLL THE DICE", 0
s_h3:       DB  "3 9 OTHER CARDS", 0
s_h4:       DB  "7 SCORES", 0
s_h5:       DB  "0 MENU", 0
s_h6:       DB  "INVERSE LABEL =", 0
s_h7:       DB  "YOU CAN SCORE IT", 0

; the card's labels, two cells a slot, in the wire's order
labels:     DB  "1S2S3S4S5S6SUPBN3K4KFHSRLRCHFZTT"
