lsqecc_slicer -i cases/end_to_end/grover/grover_3.lli --compactlayout --noslices | \
  sed "s/Made patch computation. Took [0-9]*.[0-9e\-]*s./Made patch computation. Took <time_removed_by_case_script>/"