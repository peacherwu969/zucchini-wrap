file(REMOVE_RECURSE
  "libzucchini.a"
  "libzucchini.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/zucchini_static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
