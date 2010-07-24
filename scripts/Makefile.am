EXTRA_DIST=chronos_compile.in  \
           chronos_analyze.in  \
           chronos_simulate.in

CHRONOS_SCRIPT=$(basename $(EXTRA_DIST))

.PHONY: all check_executability

all: check_executability

# Make all the scripts executable
check_executability:
	@for script in $(CHRONOS_SCRIPT); do \
	   chmod u+x $$script; \
	 done;

