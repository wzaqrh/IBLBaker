# Install script for directory: D:/proj/IBLBaker/src/critter/dependencies/libzip/man

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/IBLBaker")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man1" TYPE FILE RENAME "zipcmp.1" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zipcmp.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man1" TYPE FILE RENAME "zipmerge.1" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zipmerge.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man1" TYPE FILE RENAME "ziptorrent.1" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/ziptorrent.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "libzip.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/libzip.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_add.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_add.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_add_dir.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_add_dir.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_close.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_close.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_delete.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_delete.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_dir_add.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_dir_add.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_discard.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_discard.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_error_clear.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_clear.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_error_get.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_get.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_error_get_sys_type.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_get_sys_type.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_error_to_str.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_to_str.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_errors.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_errors.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fclose.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fclose.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fdopen.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fdopen.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_add.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_add.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_field_delete.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_field_delete.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_field_get.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_field_get.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_field_set.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_field_set.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_fields_count.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_fields_count.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_get_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_get_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_rename.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_rename.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_set_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_set_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_strerror.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_strerror.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fopen.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fopen.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fopen_encrypted.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fopen_encrypted.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fread.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fread.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_archive_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_archive_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_archive_flag.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_archive_flag.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_file_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_file_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_name.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_name.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_num_entries.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_num_entries.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_get_num_files.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_get_num_files.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_name_locate.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_name_locate.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_open.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_open.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_rename.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_rename.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_set_archive_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_set_archive_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_set_archive_flag.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_set_archive_flag.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_set_default_password.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_set_default_password.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_set_file_comment.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_set_file_comment.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_set_file_compression.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_set_file_compression.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_buffer.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_buffer.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_file.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_file.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_filep.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_filep.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_free.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_free.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_function.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_function.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_source_zip.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_source_zip.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_stat.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_stat.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_stat_init.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_stat_init.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_unchange.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_unchange.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_unchange_all.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_unchange_all.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_unchange_archive.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_unchange_archive.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_error_clear.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_clear.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_error_get.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_error_get.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_replace.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_add.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_field_delete_by_id.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_field_delete.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_field_get_by_id.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_field_get.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_file_extra_fields_count_by_id.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_extra_fields_count.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fopen_index.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fopen.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_fopen_index_encrypted.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_fopen_encrypted.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_replace.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_add.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_stat_index.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_stat.man")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man3" TYPE FILE RENAME "zip_strerror.3" FILES "D:/proj/IBLBaker/src/critter/dependencies/libzip/man/zip_file_strerror.man")
endif()

