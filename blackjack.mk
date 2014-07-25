UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
 ECHO:=echo -e '\e
endif

ifeq ($(UNAME_S),Darwin)
 ECHO:=echo '\x1B
endif


t_blk=$(shell $(ECHO) [0;30m')
t_red=$(shell $(ECHO)[0;31m')
t_grn=$(shell $(ECHO)[0;32m')
t_ylw=$(shell $(ECHO)[0;33m')
t_blu=$(shell $(ECHO)[0;34m')
t_pur=$(shell $(ECHO)[0;35m')
t_cyn=$(shell $(ECHO)[0;36m')
t_wht=$(shell $(ECHO)[0;37m')

tb_blk=$(shell $(ECHO)[1;30m')
tb_red=$(shell $(ECHO)[1;31m')
tb_grn=$(shell $(ECHO)[1;32m')
tb_ylw=$(shell $(ECHO)[1;33m')
tb_blu=$(shell $(ECHO)[1;34m')
tb_pur=$(shell $(ECHO)[1;35m')
tb_cyn=$(shell $(ECHO)[1;36m')
tb_wht=$(shell $(ECHO)[1;37m')

tu_blk=$(shell $(ECHO)[4;30m')
tu_red=$(shell $(ECHO)[4;31m')
tu_grn=$(shell $(ECHO)[4;32m')
tu_ylw=$(shell $(ECHO)[4;33m')
tu_blu=$(shell $(ECHO)[4;34m')
tu_pur=$(shell $(ECHO)[4;35m')
tu_cyn=$(shell $(ECHO)[4;36m')
tu_wht=$(shell $(ECHO)[4;37m')

bg_blk=$(shell $(ECHO)[40m')
bg_red=$(shell $(ECHO)[41m')
bg_grn=$(shell $(ECHO)[42m')
bg_ylw=$(shell $(ECHO)[43m')
bg_blu=$(shell $(ECHO)[44m')
bg_pur=$(shell $(ECHO)[45m')
bg_cyn=$(shell $(ECHO)[46m')
bg_wht=$(shell $(ECHO)[47m')
col_rst=$(shell $(ECHO)[0m')

SILENT_CC       = @echo '  $(tb_ylw)[CC]$(col_rst)       ' $(@);
SILENT_LD       = @echo '  $(tb_pur)[LD]$(col_rst)       ' $(@);
SILENT_AR       = @echo '  $(tb_cyn)[AR]$(col_rst)       ' $(@);
ANNOUNCE_TUNER	= @echo '  Now building stuff for tuner: $(tb_grn)$(*)$(col_rst)';
SILENT_INSTALL       = echo '  $(tb_grn)[INSTALL]$(col_rst)       ' $(b);

#Shut up this crap
MAKEFLAGS+=--no-print-directory


