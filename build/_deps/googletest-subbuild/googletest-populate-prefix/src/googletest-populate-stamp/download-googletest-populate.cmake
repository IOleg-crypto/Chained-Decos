# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

function(check_file_hash has_hash hash_is_good)
  if("${has_hash}" STREQUAL "")
    message(FATAL_ERROR "has_hash Can't be empty")
  endif()

  if("${hash_is_good}" STREQUAL "")
    message(FATAL_ERROR "hash_is_good Can't be empty")
  endif()

  if("" STREQUAL "")
    # No check
    set("${has_hash}" FALSE PARENT_SCOPE)
    set("${hash_is_good}" FALSE PARENT_SCOPE)
    return()
  endif()

  set("${has_hash}" TRUE PARENT_SCOPE)

  message(STATUS "verifying file...
       file='D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip'")

  file("" "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip" actual_value)

  if(NOT "${actual_value}" STREQUAL "")
    set("${hash_is_good}" FALSE PARENT_SCOPE)
    message(STATUS " hash of
    D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  does not match expected value
    expected: ''
      actual: '${actual_value}'")
  else()
    set("${hash_is_good}" TRUE PARENT_SCOPE)
  endif()
endfunction()

function(sleep_before_download attempt)
  if(attempt EQUAL 0)
    return()
  endif()

  if(attempt EQUAL 1)
    message(STATUS "Retrying...")
    return()
  endif()

  set(sleep_seconds 0)

  if(attempt EQUAL 2)
    set(sleep_seconds 5)
  elseif(attempt EQUAL 3)
    set(sleep_seconds 5)
  elseif(attempt EQUAL 4)
    set(sleep_seconds 15)
  elseif(attempt EQUAL 5)
    set(sleep_seconds 60)
  elseif(attempt EQUAL 6)
    set(sleep_seconds 90)
  elseif(attempt EQUAL 7)
    set(sleep_seconds 300)
  else()
    set(sleep_seconds 1200)
  endif()

  message(STATUS "Retry after ${sleep_seconds} seconds (attempt #${attempt}) ...")

  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "${sleep_seconds}")
endfunction()

if(EXISTS "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip")
  check_file_hash(has_hash hash_is_good)
  if(has_hash)
    if(hash_is_good)
      message(STATUS "File already exists and hash match (skip download):
  file='D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip'
  =''"
      )
      return()
    else()
      message(STATUS "File already exists but hash mismatch. Removing...")
      file(REMOVE "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip")
    endif()
  else()
    message(STATUS "File already exists but no hash specified (use URL_HASH):
  file='D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip'
Old file will be removed and new file downloaded from URL."
    )
    file(REMOVE "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip")
  endif()
endif()

set(retry_number 5)

message(STATUS "Downloading...
   dst='D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip'
   timeout='none'
   inactivity timeout='none'"
)
set(download_retry_codes 7 6 8 15 28)
set(skip_url_list)
set(status_code)
foreach(i RANGE ${retry_number})
  if(status_code IN_LIST download_retry_codes)
    sleep_before_download(${i})
  endif()
  foreach(url IN ITEMS [====[https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip]====])
    if(NOT url IN_LIST skip_url_list)
      message(STATUS "Using src='${url}'")

      
      
      
      

      file(
        DOWNLOAD
        "${url}" "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip"
        SHOW_PROGRESS
        # no TIMEOUT
        # no INACTIVITY_TIMEOUT
        STATUS status
        LOG log
        
        
        )

      list(GET status 0 status_code)
      list(GET status 1 status_string)

      if(status_code EQUAL 0)
        check_file_hash(has_hash hash_is_good)
        if(has_hash AND NOT hash_is_good)
          message(STATUS "Hash mismatch, removing...")
          file(REMOVE "D:/gitnext/Chained Decos/build/_deps/googletest-subbuild/googletest-populate-prefix/src/03597a01ee50ed33e9dfd640b249b4be3799d395.zip")
        else()
          message(STATUS "Downloading... done")
          return()
        endif()
      else()
        string(APPEND logFailedURLs "error: downloading '${url}' failed
        status_code: ${status_code}
        status_string: ${status_string}
        log:
        --- LOG BEGIN ---
        ${log}
        --- LOG END ---
        "
        )
      if(NOT status_code IN_LIST download_retry_codes)
        list(APPEND skip_url_list "${url}")
        break()
      endif()
    endif()
  endif()
  endforeach()
endforeach()

message(FATAL_ERROR "Each download failed!
  ${logFailedURLs}
  "
)
