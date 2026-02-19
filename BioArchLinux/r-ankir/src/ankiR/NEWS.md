# ankiR 0.6.0

## Major New Features

### Monte Carlo Forecasting (NEW)
* `anki_forecast_monte_carlo()` - Forecast using bootstrap simulation with 3 methods:
  - `weekday`: Preserves day-of-week patterns (recommended)
  - `block`: Preserves temporal sequences
  - `simple`: Independent sampling
* `anki_plot_monte_carlo()` - Visualize forecasts with confidence bands and simulation traces
* `anki_compare_forecasts()` - Compare Monte Carlo vs statistical methods (ARIMA, Holt-Winters)
* Features: empirical confidence intervals, probability queries (`prob_above()`, `prob_below()`), handles irregular study habits

### Burnout Detection & Review Quality (NEW)
* `anki_burnout_detection()` - Detect warning signs: declining retention, increasing response time, shorter sessions, more lapses
* `anki_review_quality()` - Detect pattern clicking, rushed reviews, suspicious timing, review bunching

### Cohort Analysis & Learning Velocity (NEW)
* `anki_cohort_analysis()` - Compare card performance by when added (vintage analysis)
* `anki_learning_velocity()` - Track cards/day, acceleration/deceleration, retention trends
* `anki_backlog_calculator()` - Calculate time to clear backlog, project backlog growth if you stop

### Gamification (NEW)
* `anki_gamification()` - XP system, levels (Novice to Grandmaster), 22 achievements, weekly goals

### Streak Analytics (NEW)
* `anki_streak_analytics()` - Best streaks, recovery time after breaks, weekend patterns, survival curves

### Card Content Analysis (NEW)
* `anki_card_content()` - Word count, cloze density, complexity score, media usage, complexity vs retention

### A/B Comparison (NEW)
* `anki_ab_comparison()` - Compare retention by note type, deck, tag, or creation period
* `anki_compare_groups()` - Statistical comparison of two groups with significance testing

### Addon Integration (NEW)
* `import_addon_export()` - Import JSON exports from ankiR Stats addon
* `analyze_addon_import()` - Analyze imported addon data

### Learning Efficiency Analysis
* `anki_learning_efficiency()` - Calculate learning ROI (successful retention per time, efficiency ratio, learning points per minute)
* `anki_retention_by_type()` - Break down retention by card characteristics (cloze vs basic, with/without media, by length, by note type)
* `anki_roi_analysis()` - Calculate knowledge half-life extension per study minute, cumulative retention value

### Forgetting Curve Analysis
* `anki_fit_forgetting_curve()` - Fit power law forgetting curve to actual review data, compare to FSRS defaults
* `anki_plot_forgetting_curve()` - Visualization comparing fitted curve to FSRS default with confidence intervals

### Optimal Review Timing
* `anki_best_review_times()` - Analyze retention/performance by hour and weekday, identify best/worst times
* `anki_session_analysis()` - Identify discrete study sessions, analyze duration/retention/fatigue patterns
* `anki_simulate_session()` - Given time budget, predict cards completed and expected retention

### Sibling & Interference Analysis
* `anki_sibling_analysis()` - Analyze how sibling cards affect each other's retention, statistical testing
* `anki_interference_analysis()` - Detect cards frequently confused (co-failure patterns, content similarity)
* `anki_weak_areas()` - Identify tags/decks with lowest retention or highest lapse rates

### Actionable Recommendations
* `anki_card_recommendations()` - Generate recommendations for leeches, unsuspend candidates, cards to retire
* `anki_health_check()` - Comprehensive health check with 0-100 score (orphan cards, leech rates, missing FSRS)
* `anki_summary()` - One-liner overview (total cards, mature cards, due today, streak, retention)
* `anki_today()` - Today's activity summary with breakdown by ease button

### Extended FSRS Analysis
* `fsrs_compare_parameters()` - Compare your FSRS parameters against defaults, flag significant differences
* `fsrs_decay_distribution()` - Analyze FSRS-6 per-card decay parameter distribution (requires Anki 24.11+)
* `fsrs_memory_states()` - Calculate current FSRS memory state for all cards with future projections
* `anki_response_time_outliers()` - Find reviews with suspicious times (too fast or too slow)

### Academic/Exam Preparation Tools
* `anki_exam_readiness()` - Project completion and retention before exam date, assess risk level
* `anki_coverage_analysis()` - Show percentage complete/mature/retained by topic
* `anki_study_priorities()` - Identify which topics should be prioritized
* `anki_study_plan()` - Generate daily study plan based on exam date and available time

### Additional Export Formats
* `anki_to_obsidian_sr()` - Export to Obsidian Spaced Repetition plugin format
* `anki_to_mochi()` - Export to Mochi Cards JSON format
* `anki_to_json()` - Full collection as structured JSON for web dashboards
* `anki_progress_report()` - Generate shareable HTML or Markdown progress report

### Enhanced Search
* `anki_search_enhanced()` - Advanced search with `added:`, `rated:`, `note:`, `card:`, `prop:`, `re:` and OR operators
* `anki_find_similar()` - Find similar cards using TF-IDF, Jaccard, or n-gram similarity

### Enhanced Forecasting
* `anki_forecast_enhanced()` - ARIMA, seasonal, and Holt-Winters forecasting with workload ceilings
* `anki_workload_projection()` - Rough workload estimates with scenario comparison (for accurate FSRS simulation, use Anki's built-in simulator or FSRS Helper add-on)
* `anki_retention_stability()` - Analyze how stable your retention is over time

### Utilities
* `anki_schema_version()` - Detect Anki database schema version for compatibility debugging
* `anki_quick_summary()` - Get a one-liner overview of your collection
* `anki_today()` - Detailed breakdown of today's Anki activity

### Removed (use r-fsrs instead)
The following algorithm functions were removed - ankiR focuses on reading/analyzing data, not implementing FSRS.
For FSRS algorithm, use [r-fsrs](https://github.com/open-spaced-repetition/r-fsrs):
* `fsrs_retrievability()` - use `fsrsr::fsrs_retrievability()`
* `fsrs_interval()` - use `fsrsr::fsrs_next_interval()`
* `fsrs_simulate()` - use Anki's built-in simulator
* `fsrs_simulate_new_deck()` - use Anki's built-in simulator
* `fsrs_time_to_mastery()` - use Anki's built-in simulator
* `fsrs_review_burden()` - use Anki's built-in simulator
* `fsrs_workload_estimate()` - use Anki's built-in simulator
* `fsrs_optimal_new_rate()` - use Anki's built-in simulator

---

# ankiR 0.5.0

## New Features

### Visualization Functions
* `anki_plot_heatmap()` - Calendar heatmap of review activity
* `anki_plot_retention()` - Retention rate over time with rolling average
* `anki_plot_forecast()` - Workload forecast chart
* `anki_plot_difficulty()` - FSRS difficulty distribution histogram
* `anki_plot_intervals()` - Card interval distribution
* `anki_plot_stability()` - FSRS stability distribution
* `anki_plot_hours()` - Reviews by hour of day
* `anki_plot_weekdays()` - Reviews by day of week

### Comparative Analysis
* `anki_compare_decks()` - Side-by-side deck statistics
* `anki_compare_periods()` - Compare two time periods
* `anki_compare_by_age()` - Retention by card age
* `anki_compare_deck_difficulty()` - Rank decks by difficulty
* `anki_benchmark()` - Compare your stats to FSRS averages

### Time Analysis
* `anki_time_by_hour()` - Detailed hourly statistics
* `anki_time_by_weekday()` - Statistics by day of week
* `anki_session_stats()` - Study session analysis
* `anki_response_time()` - Response time by difficulty/interval
* `anki_monthly_summary()` - Monthly statistics
* `anki_consistency()` - Study consistency metrics

### Card Quality Analysis
* `anki_card_complexity()` - Measure card complexity
* `anki_similar_cards()` - Find potential duplicates
* `anki_tag_analysis()` - Tag usage and retention by tag
* `anki_empty_cards()` - Find cards with empty fields
* `anki_long_cards()` - Find overly long cards
* `anki_quality_report()` - Comprehensive quality assessment

### Predictive Features
* `fsrs_time_to_mastery()` - Estimate when deck will be learned
* `fsrs_forgetting_index()` - Percentage of cards below target retention
* `fsrs_review_burden()` - Long-term daily review workload estimate
* `fsrs_optimal_new_rate()` - Sustainable new card rate recommendation
* `fsrs_simulate_new_deck()` - Project workload when adding cards

### Interactive Dashboard
* `anki_dashboard()` - Launch Shiny dashboard with all analytics

### Export Formats
* `anki_to_org()` - Export to Emacs Org-mode (org-drill)
* `anki_to_markdown()` - Export to Markdown (Obsidian/Logseq/basic)
* `anki_to_supermemo()` - Export to SuperMemo Q&A format
* `fsrs_from_csv()` - Import review data from other sources
* `anki_export_importable()` - Export to Anki-importable format

### Time Series Analysis
* `anki_ts_intervals()` - Track interval progression over time
* `anki_ts_retention()` - Track retention rate changes
* `anki_ts_stability()` - FSRS stability trends
* `anki_ts_workload()` - Workload trends over time
* `anki_ts_learning()` - New cards learned per period
* `anki_ts_maturation()` - Card maturation tracking
* `anki_ts_decompose()` - Decompose into trend/seasonal/residual
* `anki_ts_anomalies()` - Detect unusual study days
* `anki_ts_forecast()` - Forecast future reviews
* `anki_ts_autocorrelation()` - Find cyclical patterns
* `anki_ts_plot()` - Plot any time series with trend

---

# ankiR 0.4.0

## Features

### Analytics & Statistics
* `anki_cards_full()` - Cards joined with notes, decks, models
* `anki_tags()` - Unique tags with counts
* `anki_field_contents()` - Parse note fields into columns
* `anki_stats_deck()` - Per-deck statistics
* `anki_stats_daily()` - Daily review stats
* `anki_retention_rate()` - Actual retention from history
* `anki_learning_curve()` - Card progression over time
* `anki_heatmap_data()` - Calendar heatmap data
* `anki_streak()` - Current/longest streaks

### Search & Filter
* `anki_search()` - Anki-like search syntax
* `anki_suspended()`, `anki_buried()`, `anki_leeches()`
* `anki_due()`, `anki_new()`, `anki_mature()`

### Media Management
* `anki_media_list()`, `anki_media_unused()`, `anki_media_missing()`
* `anki_media_path()`, `anki_media_stats()`

### FSRS Advanced
* `fsrs_difficulty_distribution()`, `fsrs_stability_distribution()`
* `fsrs_current_retrievability()`, `fsrs_simulate()`
* `fsrs_workload_estimate()`, `fsrs_prepare_for_optimizer()`
* `fsrs_get_parameters()`

### Export
* `anki_to_csv()`, `anki_report()`, `anki_export_revlog()`, `anki_forecast()`

---

# ankiR 0.3.0

* `anki_decks()`, `anki_models()`, `fsrs_interval()`, `date_to_anki_timestamp()`
* FSRS-6 per-card decay parameter support
* Comprehensive test suite

---

# ankiR 0.2.0

* FSRS parameter support via `anki_cards_fsrs()`
* `fsrs_retrievability()` function

---

# ankiR 0.1.0

* Initial release with core functions
