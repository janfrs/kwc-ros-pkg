#ifndef WX_OGRE_RENDER_WINDOW_OGRE_RENDER_WINDOW_H_
#define WX_OGRE_RENDER_WINDOW_OGRE_RENDER_WINDOW_H_

#include "wx/wx.h"

namespace Ogre
{
class Root;
class RenderWindow;
class Viewport;
class Camera;
}

class abstractFunctor;

namespace ogre_tools
{

/** wxWidgets Ogre render window widget.
	Strongly based on the existing wxOgre widget implementation, this one
	isolates the wx component from Ogre, acting as a simple bind to render
	inside a wxWidgets window.

	@author Jesús Alonso Abad 'Kencho', Other contributors (original wxOgre).  Heavily modified by Josh Faust
 */
class wxOgreRenderWindow : public wxControl {
  DECLARE_CLASS (wxOgreRenderWindow)
  DECLARE_EVENT_TABLE ()

  // Attributes ------------------------------------------------------------------
protected:
  /// This control's own render window reference.
  Ogre::RenderWindow* render_window_;

  /// This control's pointer to the global Ogre::Root
  Ogre::Root* ogre_root_;
  
  /// This control's Viewport
  Ogre::Viewport* viewport_;

  /// The Id of the next render window
  static unsigned int sm_NextRenderWindowId;

  // Methods ---------------------------------------------------------------------
public:
  /** wx-like Constructor.
  	@param parent The parent wxWindow component.
  	@param id The control id.
  	@param pos The default position.
  	@param size The default size.
  	@param style The default style for this componOgre::SceneManager* scene_manager,ent.
  	@param validator A default validator for the component.
   */
  wxOgreRenderWindow (Ogre::Root* ogre_root, wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize,
                      long style = wxSUNKEN_BORDER, const wxValidator &validator = wxDefaultValidator);

  /** Virtual destructor.
   */
  virtual ~wxOgreRenderWindow ();
  
  /** Set the pre-render callback. */
  virtual void setPreRenderCallback( abstractFunctor* func );
  virtual void setPostRenderCallback( abstractFunctor* func );

  /** Overrides the default implementation.
  	This override is here for convenience. Returns a symbolic 320x240px size.
  	@return A size of 320x240 (just a symbolic 4:3 size).
   */
  virtual wxSize DoGetBestSize () const;

  /** Gets the associated Ogre render window.
  	@return The render window used to paint this control.
   */
  Ogre::RenderWindow* getRenderWindow () const;
  
  /** Gets the associated Ogre viewport.
    @return The viewport used to render this window.
   */
  Ogre::Viewport* getViewport() const;
  
  /** Set the camera associated with this render window's viewport.
   */
  void setCamera( Ogre::Camera* camera );

  /** Painting event callback.
  	@param evt Data regarding the painting event.
   */
  virtual void onPaint (wxPaintEvent &evt);

  /** Resizing events callback.
  	@param evt Data regarding the resize event.
   */
  virtual void onSize (wxSizeEvent &evt);

  /** Mouse events callback.
  	@remarks Note this will call the specified callback function to process
  		the event.
  	@param evt Data regarding the mouse event.
   */
  virtual void onMouseEvents (wxMouseEvent &evt);

protected:
  /** Creates an Ogre render window for this widget.
   */
  virtual void createRenderWindow ();

  /** Gets the handle for the render window.
  	@return The render window handle.
   */
  virtual std::string getOgreHandle () const;
  
  void setCameraAspectRatio();

  abstractFunctor* m_PreRenderCallback;
  abstractFunctor* m_PostRenderCallback;
};

} // namespace ogre_tools

#endif /*OGRE_RENDER_WINDOW_H_*/
