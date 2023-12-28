

add_executable(aes_iknp_test aes_iknp.cc)
add_executable(aes_2r_test aes_2r.cc)
add_executable(sha_iknp_test sha_iknp.cc)
add_executable(and_pcg_test and_pcg.cc)
add_executable(and_iknp_test and_iknp.cc)
add_executable(and_arith_test and_arith.cc)




target_link_libraries(aes_iknp_test
PUBLIC
  sif
)

target_link_libraries(aes_2r_test
PUBLIC
  sif
)

target_link_libraries(sha_iknp_test
PUBLIC
  sif
)

target_link_libraries(and_arith_test
PUBLIC
  sif
)

target_link_libraries(and_pcg_test
PUBLIC
  sif
)
target_link_libraries(and_iknp_test
PUBLIC
  sif
)