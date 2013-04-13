/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * URLListVerifier.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef SLP_URLLISTVERIFIER_H_
#define SLP_URLLISTVERIFIER_H_

/**
 * This class is used for testing and allows us to verify the results of a
 * FindService callback.
 */
class URLListVerifier {
  public:
    typedef ola::BaseCallback1<void, const URLEntries&> FindServiceCallback;

    explicit URLListVerifier(const URLEntries &expected_urls)
        : m_callback(ola::NewCallback(this, &URLListVerifier::NewServices)),
          m_expected_urls(expected_urls),
          m_received_callback(false) {
    }

    ~URLListVerifier() {
      if (!std::uncaught_exception())
        OLA_ASSERT_TRUE(m_received_callback);
    }

    FindServiceCallback *GetCallback() const { return m_callback.get(); }

    void Reset() {
      m_received_callback = false;
    }

    bool CallbackRan() const { return m_received_callback; }

    void NewServices(const URLEntries &urls) {
      OLA_ASSERT_VECTOR_EQ(m_expected_urls, urls);
      m_received_callback = true;
    }

  private:
    auto_ptr<FindServiceCallback> m_callback;
    const URLEntries m_expected_urls;
    bool m_received_callback;
};

#endif  // SLP_URLLISTVERIFIER_H_
