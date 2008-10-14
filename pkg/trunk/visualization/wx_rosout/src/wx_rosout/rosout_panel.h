/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * wx panel for viewing rosout.
 *
 * Written by Josh Faust
 */
#ifndef WX_ROSOUT_ROSOUT_PANEL
#define WX_ROSOUT_ROSOUT_PANEL

/**
@mainpage

@htmlinclude manifest.html

*/

#include "rosout_generated.h"
#include "rostools/Log.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <rosthread/mutex.h>

#include <boost/function.hpp>

#include <wx/font.h>

namespace ros
{
class node;
}

class wxTimer;
class wxTimerEvent;
class wxAuiNotebook;
class wxRichTextCtrl;

namespace wx_rosout
{

/**
 * \class RosoutPanel
 * \brief An embeddable panel which listens on rosout and displays any messages that arrive.
 */
class RosoutPanel : public RosoutPanelBase
{
public:
  /**
   * \brief Constructor
   * @param parent The window which is the parent of this one
   */
  RosoutPanel( wxWindow* parent );
  ~RosoutPanel();

  /**
   * \brief Set this panel to be enabled or not.
   *
   * When enabled, it will be subscribed to the rosout topic and processing messages.  When disabled, it will not.
   * @param enabled Should we be enabled?
   */
	void setEnabled(bool enabled);
	/**
	 * \brief Set the topic to listen on for rostools::Log messages
	 * @param topic The topic name
	 */
  void setTopic( const std::string& topic );

  /**
   * \brief Clear all messages
   */
  void clear();

  /**
   * \brief Set the number of messages to display before we start throwing away old ones
   * @param size The number of messages
   */
  void setBufferSize( uint32_t size );

protected:
  /**
   * \brief (wx callback) Called when the "Setup" button is pressed
   */
  virtual void onSetup( wxCommandEvent& event );
  /**
   * \brief (wx callback) Called when the "Pause" button is pressed
   */
  virtual void onPause( wxCommandEvent& event );
  /**
   * \brief (wx callback) Called when the "Clear" button is pressed
   */
  virtual void onClear( wxCommandEvent& event );
  /**
   * \brief (wx callback) Called every 100ms so we can process new messages
   */
  void onProcessTimer( wxTimerEvent& evt );

  virtual void onFilterText( wxCommandEvent& event );

  /**
   * \brief subscribe to our topic
   */
  void subscribe();
  /**
   * \brief unsubscribe from our topic
   */
  void unsubscribe();

  /**
   * \brief (ros callback) Called when there is a new message waiting
   */
  void incomingMessage();
  /**
   * \brief Processes any messages in our message queue
   */
  void processMessages();
  /**
   * \brief Process a log message
   * @param message The message to process
   */
  void processMessage( const rostools::Log& message );
  /**
   * \brief Add a message to the table
   * @param message The message
   * @param id The unique id of the message
   */
  void addMessageToTable( const rostools::Log& message, uint32_t id );

  /**
   * \brief Filter a string based on our current filter
   * @param str The string to match against
   * @return True if the string matches, false otherwise
   */
  bool filter( const std::string& str ) const;
  typedef std::vector<std::string> V_string;
  /**
   * \brief Filter a vector of strings based on our current filter
   * @param strs The strings to match against
   * @return True of any string matches, false otherwise
   */
  bool filter( const V_string& strs ) const;
  /**
   * \brief Filter a message based on our current filter
   * @param id The id of the message to filter
   * @return True of anything in the message matches, false otherwise
   */
  bool filter( uint32_t id ) const;
  /**
   * \brief Re-filter all messages
   * @param old_filter The previous filter -- used for optimization in the case where the filter is growing in length
   */
  void refilter( const std::string& old_filter );

  /**
   * \brief Get a message by index in our ordered message list.  Used by the list control.
   * @param index Index of the message to return
   * @return The message
   */
  const rostools::Log& getMessageByIndex( uint32_t index ) const;

  /**
   * \brief Remove The oldest message
   */
  void popMessage();


  bool enabled_;                                            ///< Are we enabled?
  std::string topic_;                                       ///< The topic we're listening on (or will listen on once we're enabled)

  ros::node* ros_node_;                                     ///< Our pointer to the global ros::node
  rostools::Log message_;                                   ///< Our incoming message

  typedef std::vector<rostools::Log> V_Log;
  V_Log message_queue_;                                     ///< Queue of messages we've received since the last time processMessages() was called
  ros::thread::mutex queue_mutex_;                          ///< Mutex for locking the message queue

  wxTimer* process_timer_;                                  ///< Timer used to periodically process messages

  uint32_t message_id_counter_;                             ///< Counter for generating unique ids for messages
  typedef std::map<uint32_t, rostools::Log> M_IdToMessage;
  M_IdToMessage messages_;                                  ///< Map of id->message
  std::string filter_;                                      ///< String to filter what's displayed in the list by

  typedef std::vector<uint32_t> V_u32;
  V_u32 ordered_messages_;                                  ///< Already-filtered messages that are being displayed in the list

  uint32_t max_messages_;                                   ///< Max number of messages to keep around.  When we hit this limit, we start throwing away the oldest messages
};

} // namespace wx_rosout

#endif // WX_ROSOUT_ROSOUT_PANEL
