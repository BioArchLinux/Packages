test_that("anki_timestamp_to_datetime converts correctly", {
  # Known timestamp: 2013-05-11 14:25:17 UTC
  ts <- 1368282317000
  dt <- anki_timestamp_to_datetime(ts)

  expect_s3_class(dt, "POSIXct")
  expect_equal(as.numeric(dt), 1368282317, tolerance = 1)
})

test_that("anki_timestamp_to_date converts correctly", {
  ts <- 1368282317000
  d <- anki_timestamp_to_date(ts)

  expect_s3_class(d, "Date")
  expect_equal(as.character(d), "2013-05-11")
})

test_that("date_to_anki_timestamp converts correctly", {
  d <- as.Date("2013-05-11")
  ts <- date_to_anki_timestamp(d)

  expect_type(ts, "double")
  # Should be midnight UTC on that date (within a day's worth of milliseconds)
  expect_true(abs(ts - 1368230400000) < 86400000)
})

test_that("timestamp conversions are reversible", {
  original_ts <- 1609459200000  # 2021-01-01 00:00:00 UTC
  dt <- anki_timestamp_to_datetime(original_ts)
  back_ts <- date_to_anki_timestamp(dt)

  expect_equal(back_ts, original_ts, tolerance = 1000)  # Within 1 second
})

test_that("timestamp functions are vectorized
", {
  timestamps <- c(1368282317000, 1609459200000, 1640995200000)

  datetimes <- anki_timestamp_to_datetime(timestamps)
  expect_length(datetimes, 3)

  dates <- anki_timestamp_to_date(timestamps)
  expect_length(dates, 3)
})
