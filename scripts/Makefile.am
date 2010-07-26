EXTRA_DIST=chronos_compile.in  \
           chronos_analyze.in  \
           chronos_simulate.in

CHRONOS_SCRIPT=chronos_compile  \
							 chronos_analyze  \
							 chronos_simulate

.PHONY: all check_executability

all: check_executability

# Make all the scripts executable
check_executability:
	@for script in $(CHRONOS_SCRIPT); do \
	   chmod u+x $$script; \
	 done;

