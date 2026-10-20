# Analysis pipeline: raw logs -> host recompute -> tables -> figures -> paper tables.
#   make analysis     (needs Python 3; figures need matplotlib, e.g. in .venv)
#   make host         (build the host test program)
#   make paper-zip    (paper/ folder as a zip for Overleaf)
PY      ?= python3
PYPLOT  ?= .venv/bin/python

analysis:
	$(PY) analysis/recompute.py $$($(PY) -c "import sys; sys.path.insert(0,'analysis'); from summarize import raw_logs; print(' '.join(sorted(raw_logs())))") > results/summary/recompute.txt; tail -1 results/summary/recompute.txt
	$(PY) analysis/summarize.py > /dev/null
	$(PY) analysis/footprint.py > /dev/null
	$(PYPLOT) analysis/figures.py
	$(PY) analysis/paper_tables.py > /dev/null
	rm -f paper/figures/*.pdf && cp figures/paper_fig1_maxt_testA.pdf figures/paper_fig2_cost.pdf paper/figures/

host:
	$(MAKE) -C harness/posix

paper-zip:
	cd paper && rm -f ../paper_overleaf.zip && zip -qr ../paper_overleaf.zip paper.tex logo.eps figures tables && cd .. && ls -la paper_overleaf.zip

.PHONY: analysis host paper-zip
