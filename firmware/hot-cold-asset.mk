# MIT License

# Copyright (c) 2024 Jerry Lum

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Modified from:
# https://github.com/Jerrylum/hot-cold-asset/blob/9638cca75a8d6ea93b6ddf5c767d2d023e1803cf/hot-cold-asset-repo/firmware/hot-cold-asset.mk

ASSET_DIRS=static static.lib

ASSET_FILES:=$(shell mkdir -p $(ASSET_DIRS) && find $(ASSET_DIRS) -type f)

ASSET_OBJ=$(addprefix $(BINDIR)/, $(addsuffix .o, $(ASSET_FILES)) )

GETALLOBJ=$(sort $(call ASMOBJ,$1) $(call COBJ,$1) $(call CXXOBJ,$1)) $(ASSET_OBJ)

.SECONDEXPANSION:
$(ASSET_OBJ): $$(patsubst bin/%,%,$$(basename $$@))
	$(VV)mkdir -p $(dir $@)
	@echo "ASSET $@"
	$(VV)$(OBJCOPY) -I binary -O elf32-littlearm -B arm $^ $@