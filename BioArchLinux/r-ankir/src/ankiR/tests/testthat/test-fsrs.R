test_that("fsrs_retrievability calculates correctly", {
  # When days_elapsed = stability, retrievability should be ~0.9
  r <- fsrs_retrievability(stability = 30, days_elapsed = 30, decay = 0.5)
  expect_equal(r, 0.9, tolerance = 0.001)

  # At t=0, retrievability should be 1
  r0 <- fsrs_retrievability(stability = 30, days_elapsed = 0, decay = 0.5)
  expect_equal(r0, 1.0, tolerance = 0.001)

  # Retrievability should decrease with time
  r1 <- fsrs_retrievability(stability = 30, days_elapsed = 15, decay = 0.5)
  r2 <- fsrs_retrievability(stability = 30, days_elapsed = 60, decay = 0.5)
  expect_true(r1 > r2)
  expect_true(r1 < 1 && r1 > 0.9)
  expect_true(r2 < 0.9 && r2 > 0)

  # Higher stability = slower decay
  r_high_s <- fsrs_retrievability(stability = 100, days_elapsed = 30, decay = 0.5)
  r_low_s <- fsrs_retrievability(stability = 10, days_elapsed = 30, decay = 0.5)
  expect_true(r_high_s > r_low_s)
})

test_that("fsrs_retrievability handles different decay values", {
  # All decay values should give R=0.9 when t=S
  for (decay in c(0.1, 0.3, 0.5, 0.8)) {
    r <- fsrs_retrievability(stability = 30, days_elapsed = 30, decay = decay)
    expect_equal(r, 0.9, tolerance = 0.001,
                 info = paste("decay =", decay))
  }

  # Higher decay = faster forgetting (steeper curve at t > S)
  r_low_decay <- fsrs_retrievability(stability = 30, days_elapsed = 60, decay = 0.2)
  r_high_decay <- fsrs_retrievability(stability = 30, days_elapsed = 60, decay = 0.8)
  expect_true(r_low_decay > r_high_decay)
})

test_that("fsrs_retrievability is vectorized", {
  stabilities <- c(10, 30, 100)
  days <- c(5, 15, 50)
  result <- fsrs_retrievability(stabilities, days)

  expect_length(result, 3)
  expect_true(all(result > 0 & result <= 1))
})

test_that("fsrs_interval calculates correctly", {
  # At desired_retention = 0.9, interval should equal stability
  ivl <- fsrs_interval(stability = 30, desired_retention = 0.9, decay = 0.5)
  expect_equal(ivl, 30, tolerance = 0.01)

  # Lower retention = longer interval
  ivl_high <- fsrs_interval(stability = 30, desired_retention = 0.95, decay = 0.5)
  ivl_low <- fsrs_interval(stability = 30, desired_retention = 0.80, decay = 0.5)
  expect_true(ivl_high < 30)
  expect_true(ivl_low > 30)

  # Higher stability = longer interval
  ivl_s10 <- fsrs_interval(stability = 10, desired_retention = 0.9, decay = 0.5)
  ivl_s100 <- fsrs_interval(stability = 100, desired_retention = 0.9, decay = 0.5)
  expect_equal(ivl_s10, 10, tolerance = 0.01)
  expect_equal(ivl_s100, 100, tolerance = 0.01)
})

test_that("fsrs_interval and fsrs_retrievability are inverses", {
  stability <- 30
  desired_retention <- 0.85
  decay <- 0.4

  # Calculate interval for desired retention
  interval <- fsrs_interval(stability, desired_retention, decay)

  # Calculate retrievability at that interval
  r <- fsrs_retrievability(stability, interval, decay)

  expect_equal(r, desired_retention, tolerance = 0.001)
})
