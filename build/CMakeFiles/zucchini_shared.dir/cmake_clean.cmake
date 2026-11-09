file(REMOVE_RECURSE
  ".1"
  "libzucchini.pdb"
  "libzucchini.so"
  "libzucchini.so.1"
  "libzucchini.so.1.0"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/zucchini_shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
