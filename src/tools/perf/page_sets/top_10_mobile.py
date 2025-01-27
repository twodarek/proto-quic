# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import cache_temperature as cache_temperature_module
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


URL_LIST = [
    # Why: #1 (Alexa) most visited page worldwide, picked a reasonable
    # search term
    'https://www.google.co.uk/#hl=en&q=science',
    # Why: #2 (Alexa) most visited page worldwide, picked the most liked
    # page
    'https://m.facebook.com/rihanna',
    # Why: #3 (Alexa) most visited page worldwide, picked a reasonable
    # search term
    'http://m.youtube.com/results?q=science',
    # Why: #4 (Alexa) most visited page worldwide, picked a reasonable search
    # term
    'http://search.yahoo.com/search;_ylt=?p=google',
    # Why: #5 (Alexa) most visited page worldwide, picked a reasonable search
    # term
    'http://www.baidu.com/s?word=google',
    # Why: #6 (Alexa) most visited page worldwide, picked a reasonable page
    'http://en.m.wikipedia.org/wiki/Science',
    # Why: #10 (Alexa) most visited page worldwide, picked the most followed
    # user
    'https://mobile.twitter.com/justinbieber?skip_interstitial=true',
    # Why: #11 (Alexa) most visited page worldwide, picked a reasonable
    # page
    'http://www.amazon.com/gp/aw/s/?k=nexus',
    # Why: #13 (Alexa) most visited page worldwide, picked the first real
    # page
    'http://m.intl.taobao.com/group-purchase.html',
    # Why: #18 (Alexa) most visited page worldwide, picked a reasonable
    # search term
    'http://yandex.ru/touchsearch?text=science',
]


class Top10MobilePage(page_module.Page):
  def __init__(self, url, page_set, cache_temperature=None):
    super(Top10MobilePage, self).__init__(
        url=url, page_set=page_set, credentials_path='data/credentials.json',
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        cache_temperature=cache_temperature, name=url)
    self.archive_data_file = 'data/top_10_mobile.json'


class Top10MobilePageSet(story.StorySet):
  """Top 10 mobile sites.

  Note: this class is being deprecated, currently only used by
  page_cycler_v2.top_10_mobile (contrib benchmark).
  """
  def __init__(self, cache_temperatures=None):
    super(Top10MobilePageSet, self).__init__(
      archive_data_file='data/top_10_mobile.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)
    if cache_temperatures is None:
      cache_temperatures = [cache_temperature_module.ANY]

    for url in URL_LIST:
      for temp in cache_temperatures:
        self.AddStory(Top10MobilePage(url, self, cache_temperature=temp))


class Top10MobileStoryExpectations(story.expectations.StoryExpectations):
  def SetExpectations(self):
    pass
