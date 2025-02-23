################################################################################
######################### User configurable parameters #########################
# filename extensions
CEXTS:=c
ASMEXTS:=s S
CXXEXTS:=cpp c++ cc

# probably shouldn't modify these, but you may need them below
ROOT=.
FWDIR:=$(ROOT)/firmware
BINDIR=$(ROOT)/bin
SRCDIR=$(ROOT)/src
INCDIR=$(ROOT)/include
BUILDDIR=$(ROOT)/build

# TODO: Create a rule to copy mcap, flatbuffers, and lz4 headers from their respective repositories

# ----
# Options for the schemas repo
# ----

SCHEMAS_REPO_URL=https://github.com/foxglove/foxglove-sdk.git
SCHEMAS_TAG=releases/python/foxglove-schemas-protobuf/v0.3.0
# Download path for schemas repo
SCHEMAS_PATH=$(BUILDDIR)/foxglove-sdk
# Where to be put the generated code files
SCHEMAS_GEN_OUT_PATH=$(INCDIR)/foxglove
# Path to the flatbuffer schema files, relative to the root of the schemas repo
FBS_PATH=schemas/flatbuffer
FLATC_COMMAND=flatc

WARNFLAGS+=

EXTRA_C_BOTH_FLAGS=-isystem $(SRCDIR)/lz4

EXTRA_CFLAGS=$(EXTRA_C_BOTH_FLAGS)
EXTRA_CXXFLAGS=$(EXTRA_C_BOTH_FLAGS)

# Set to 1 to enable hot/cold linking
USE_PACKAGE:=1

# Add libraries you do not wish to include in the cold image here
# EXCLUDE_COLD_LIBRARIES:= $(FWDIR)/your_library.a
EXCLUDE_COLD_LIBRARIES:= 

# Set this to 1 to add additional rules to compile your project as a PROS library template
IS_LIBRARY:=1
# TODO: CHANGE THIS! 
# Be sure that your header files are in the include directory inside of a folder with the
# same name as what you set LIBNAME to below.
LIBNAME:=mcap
VERSION:=0.2.0
# EXCLUDE_SRC_FROM_LIB= $(SRCDIR)/unpublishedfile.c
# this line excludes opcontrol.c and similar files
EXCLUDE_SRC_FROM_LIB+=$(foreach file, $(SRCDIR)/main,$(foreach cext,$(CEXTS),$(file).$(cext)) $(foreach cxxext,$(CXXEXTS),$(file).$(cxxext)))

# files that get distributed to every user (beyond your source archive) - add
# whatever files you want here. This line is configured to add all header files
# that are in the directory include/LIBNAME
TEMPLATE_FILES=$(INCDIR)/$(LIBNAME)/*.h $(INCDIR)/$(LIBNAME)/*.hpp

# Add foxglove schema files
TEMPLATE_FILES+=$(INCDIR)/foxglove/*.h $(INCDIR)/foxglove/LICENSE.md

# Add flatbuffers library files
TEMPLATE_FILES+=$(INCDIR)/flatbuffers/*.h $(INCDIR)/flatbuffers/pch/*.h

# Add makefile extension for no-zstd flag
TEMPLATE_FILES+=$(FWDIR)/mcap-no-zstd.mk

################################################################################
################################################################################
########## Nothing below this line should be edited by typical users ###########
-include ./common.mk

.PHONY: clean gen_schema_code clean_gen_schema_code

# Override clean to remove build directory
clean:: clean_gen_schema_code
	-$Drm -rf $(BUILDDIR)

# Override the default template rule to build the foxglove schemas
template:: gen_schema_code clean-template $(LIBAR)
	@echo "Creating template"
	$Dpros c create-template . $(LIBNAME) $(VERSION) $(foreach file,$(TEMPLATE_FILES) $(LIBAR),--system "$(file)") --target v5 $(CREATE_TEMPLATE_FLAGS)

# Clean generated schema code
clean_gen_schema_code:
	@echo Cleaning generated schema code
	-$Drm -rf $(SCHEMAS_PATH)
	-$Drm -rf $(SCHEMAS_GEN_OUT_PATH)/{LICENSE.md,*_generated.h}

# TODO: Create rule for:
# flatc --binary --schema -o ./static.lib -I build/foxglove-sdk/schemas/flatbuffer build/foxglove-sdk/schemas/flatbuffer/*.fbs

# Generate code from foxglove flatbuffer schemas
gen_schema_code: clean_gen_schema_code
	-$D# Install flatc if it is not already installed
	-$D./scripts/install_flatc.sh

	-$D# Install 
	@echo Cloning schemas repo
	-$Dgit clone $(SCHEMAS_REPO_URL) --depth=1 --no-checkout --branch=$(SCHEMAS_TAG) $(SCHEMAS_PATH) 2> /dev/null
	-$Dgit -C $(SCHEMAS_PATH) sparse-checkout init --cone
	-$Dgit -C $(SCHEMAS_PATH) checkout tags/$(SCHEMAS_TAG) 2> /dev/null
	-$Dgit -C $(SCHEMAS_PATH) sparse-checkout set $(FBS_PATH)
	
	@echo Compiling schemas into generated code
	-$Dflatc --cpp -o $(SCHEMAS_GEN_OUT_PATH) -I $(SCHEMAS_PATH)/$(FBS_PATH) $(SCHEMAS_PATH)/$(FBS_PATH)/*.fbs
	-$Dcp $(SCHEMAS_PATH)/LICENSE.md $(SCHEMAS_GEN_OUT_PATH)
