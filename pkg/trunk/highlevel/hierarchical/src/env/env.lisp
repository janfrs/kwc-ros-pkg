(in-package env)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; class definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defclass <env> ()
  ((state :writer set-state
	  :reader state
	  :initarg :state)
   (last-percept :reader last-percept
		 :writer set-last-percept))
		 
  (:documentation "Abstract class <env>.  Defines an environment where agents do actions and get reward.  See documentation for env-user and create-env packages."))

(defmethod initialize-instance :after ((e <env>) &rest args &key (initialize-state t))
  (declare (ignore args))
  (when initialize-state
    (reset e)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; exported operations for the outside world
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



(defgeneric get-last-percept (e)
  (:documentation "get-last-percept E.  Return the last percept of E.  The return value should be treated as immutable, since the environment might hold a reference to it.")
  (:method ((e <env>))
	   (last-percept e)))
  
(defgeneric do-action (e a)
  (:documentation "do-action E A.  Does action A in environment E, and returns three values : 1) Reward 2) Percept 3) Have we reached a terminal state?  The returned percept should be treated as immutable, since the environment might hold a reference to it.")
  (:method ((e <env>) a)
	   (assert (not (at-terminal-state e)) nil "Tried to do an action in a terminal state")
	   (multiple-value-bind (new-state reward)
	       (sample-next e (state e) a)
	     (set-last-percept (sample-percept e (state e) a reward new-state) e)
	     (set-state new-state e)
	     (values reward (get-last-percept e) (at-terminal-state e)))))


(defgeneric reset (e &optional reset-to-nonterminal-state)
  (:documentation "reset E &optional (reset-to-nonterminal-state T).  Reset the state of the environment by sampling from the init state distribution.  If reset-to-nonterminal-state is nil, just return the first sampled state.  If it's true, return the first one that is not terminal.  Assert if unable to sample a nonterminal state within 100 tries.")
  (:method ((e <env>) &optional (reset-to-nonterminal-state t))
	   (loop
	       for i from 0
	       for s = (sample-init e)
	       while (and reset-to-nonterminal-state (is-terminal-state e s))
	       do (assert (< i 100) nil "Failed to reset environment to a nonterminal state.")
	       finally (set-state s e)
		       (set-last-percept (sample-init-percept e s) e)
		       (values))))


(defgeneric reset-to-state (e s)
  (:documentation "reset-to-state E S.  Reset the state of the environment to a particular state S and return it.  Not all subclasses will implement this.")
  (:method :after ((e <env>) s) 
	   (declare (ignore s))
	   (set-last-percept (sample-init-percept e (state e)) e)))


;; TODO delete
(defgeneric io-interface (e)
  (:documentation "io-interface E.  Read actions from the terminal and perform them in E, until the user types ``nil'' for the action"))


(defgeneric at-terminal-state (e)
  (:documentation "at-terminal-state E.  Is the current state of E terminal?")
  (:method ((e <env>)) (is-terminal-state e (state e))))

(defgeneric is-terminal-state (e s)
  (:documentation "is-terminal-state E S.  Is S a terminal state of E?")
  (:method ((e <env>) s) (declare (ignore s)) nil))


(defun get-actions (e)
  "get-actions E.  Return set of available actions (currently, a vector or list) at current state of E.  May also return nil for environments that fail to implement the avail-actions method."
  (avail-actions e (state e)))


(defun current-effectors (e)
  "current-effectors E.  Return list of effectors at current state of E."
  (effectors (get-state e)))


(defgeneric same-state (s1 s2)
  (:documentation "same-state STATE1 STATE2.  STATE1 and STATE2 represent states of an environment.  Return true if they represent the same state.  Defaults to #'equal")
  (:method (s1 s2) (equal s1 s2)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Translating actions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define-condition untranslatable-action (error)
  ((action :initarg :action :reader action)
   (state :initarg :state :reader state)
   (env :initarg :env :reader env))
  (:report (lambda (c s) (format s "~a does not designate a valid action at state ~a of ~a" (action c) (state c) (env c)))))


(defgeneric translate-action (e a state choices)
  (:documentation "translate-action ENV ACTION-DESIGNATOR STATE &optional CHOICES

ACTION-DESIGNATOR is an object designating an action in a manner that depends on the particular environment and state.  This function should return the designated action, or signal an 'untranslatable-action error.  A top-level around method first sets choices if it is nil, then checks if the designator is already a member of the choice set and just returns it in that case.")
  (:method :around (e a state choices)
	   (orf choices (avail-actions e state))
	   (if (set:member? a choices)
	       a
	     (handler-case
		 (call-next-method e a state choices)
	       (untranslatable-action ()
		 (error 'untranslatable-action :action a :state state :env e)))))
  (:method (e a state choices)
	   (declare (ignore a e state choices))
	   (error 'untranslatable-action)))





;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; operations to be overridden by child classes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



(defgeneric sample-next (e s a)
  (:documentation "sample-next E S A.  Samples from the next-state distribution of E given state S and action A (note that it doesn't modify the state of E).  Returns two values - the new state, and the reward.

Implementors must make sure not to change S directly, or any of the objects that S refers to, but instead create a new object for the new state, since other code could be holding references to S.  
"))


(defgeneric sample-init (e)
  (:documentation "sample-init E.  Sample a state from the initial state distribution of this environment."))

(defgeneric sample-percept (e s a r next-s)
  (:documentation "sample-percept E S A R S2.  Sample a percept given that we just did A and went from S to S2 receiving R."))

(defgeneric sample-init-percept (e s)
  (:documentation "sample-init-percept E S.  Sample a percept given that the environment was just reset to S."))
	   

(defgeneric avail-actions (e s)
  (:documentation "avail-actions E S.  Return set of actions available in E at state S.  Subclasses may decline to implement this, in which case the top-level version, which returns the universal set, is called.")
  (:method ((e <env>) s) (declare (ignore s)) t))

(defgeneric effectors (s)
  (:documentation "effectors S.  Return set of effectors in state S.  The effector set may differ across different states in the same environment.  If the same effector ID occurs in consecutive states in a given environment, the corresponding effectors are assumed to be 'the same'."))


  
(in-package common-lisp-user)
	    
	    
	   


