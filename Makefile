APP       = conan

GENERATOR = mapgen.py
PICTURE   = img/$(APP).png
GENERATED = $(APP)_map.gen.h
PYTHON    = python


include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o cube.o main.o
ASSETDEPS += img/*.png $(ASSETS).lua

include $(SDK_DIR)/Makefile.rules


# make clean++
cleanall :: clean
	rm -f $(GENERATED)

# generate a header with a binary map of conan background
$(GENERATED): $(PICTURE) $(GENERATOR)
	$(PYTHON) $(GENERATOR) $< > $@

# my "make run" ;)
simu :: $(GENERATED) all run

