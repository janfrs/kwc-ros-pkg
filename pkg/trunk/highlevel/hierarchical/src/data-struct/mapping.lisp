(defpackage mapping
  (:documentation "Package mapping.  Abstracts different types of mappings.

Types
-----
[mapping]
[changeable-mapping]

Operations
----------
evaluate
evaluate-mv
defined?
set-value
do-entries
make-undefined
domain
transform

Conditions
----------
mapping-undefined
")



  (:use 
   cl
   utils
   set)
  (:export
   evaluate
   evaluate-mv
   defined?
   set-value
   do-entries
   make-undefined
   domain
   transform
   
   [mapping]
   [changeable-mapping]
   
   mapping-undefined)
  )


(in-package mapping)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; type defs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(deftype [changeable-mapping] ()
  "A changeable mapping is a [mapping] that can be changed.  Either a sequence or hashtable."
  `(or sequence hash-table))

(deftype [mapping] ()
  "Type for mappings.  Can be one of
1) A function of one argument that takes in an item and returns the corresponding value.  The function is required to signal a 'mapping-undefined error if given an item at which it is undefined.
2) A list, which is nil, or whose first element is a pair.  This is interpreted as a #'equal association list.
3) A list whose first element is a function.  The rest of this list is interpreted as an association list where elements are compared using the function.
4) A vector.  This is interpreted as mapping from 0,1,...,n-1 to the elements of the sequence.
5) A hashtable

See also [changeable-mapping]."
  `(or function sequence hash-table))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; definedness
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define-condition mapping-undefined ()
  ((item :initarg :item :reader get-item)
   (m :initarg :mapping :reader get-mapping))
  (:report (lambda (c s) (format s "Value of mapping ~a not defined at ~a" (get-mapping c) (get-item c))))
  (:documentation "Signalled when applying a mapping to an item that it's not defined at."))


(defun defined? (m x)
  "defined? MAPPING ITEM.  Return t iff mapping is defined at item?"
  (handler-case 
      (progn (evaluate m x) t)
    (mapping-undefined ()
      nil)))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; getting values
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
(defgeneric evaluate (m x)
  (:documentation "Evaluate MAPPING ITEM.  Return value of MAPPING applied to item.  Signals a 'mapping-undefined error if item is not present.  Is an accessor, i.e., something like (setf (evaluate m x) y) works.")
  (:method ((m function) x)
	   (funcall m x))
  (:method ((m list) x)
	   (condlet
	    (((is-assoc-list m) (l m) (test #'equal))
	     (t (l (cdr m)) (test (car m))))
	    (check-type test function)
	    (aif (assoc x l :test test)
		(cdr it)
	      (error 'mapping-undefined :item x :mapping m))))
  (:method ((m vector) x)
	   (if (and (typep x 'integer)
		    (between x 0 (length m)))
	       (elt m x)
	     (error 'mapping-undefined :item x :mapping m)))
			    
  (:method ((m hash-table) x)
	   (mvbind (val present?)
	       (gethash x m)
	     (if present?
		 val
	       (error 'mapping-undefined :item x :mapping m)))))

(defun evaluate-mv (m x)
  "evaluate-mv MAPPING ITEM.  Apply evaluate to MAPPING and ITEM, and handle errors. Works like gethash - returns two values: 1) the mapping value if it exists, or nil otherwise 2) t if the item was present and nil otherwise."
  (handler-case
      (values (evaluate m x) t)
    (mapping-undefined ()
      (values nil nil))))

(defsetf evaluate set-mapping-value)
  





;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; setting values
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defmacro set-value (m x y &optional (defined? nil def-supp))
  "set-value M X Y &optional DEFINED?.  

M must be of type [changeable-mapping]

Set the value of M at X to equal Y, and return Y.  M may be destructively modified.  If DEFINED? is provided, first verifies either that M is already defined at X if DEFINED? is true, or that it is not if DEFINED? is nil."
  
  `(progn
     ,@(when def-supp
	 `((assert (nxor ,defined? (defined? ,m ,x)) ()
	     "Mapping ~a is, unexpectedly, ~:[~;not ~]defined at ~a" 
	     ,m ,defined? ,x)))
     (_f set-mapping-value ,m ,x ,y)))

(defgeneric set-mapping-value (m x y)
  (:documentation "set-mapping-value M X Y.  Return a mapping that is the same as M except X now maps to Y.  M may be destructively modified.  This shouldn't be called by external code - use set-value instead.")
  (:method ((m list) x y)
	   (condlet 
	    (((is-assoc-list m) (l m) (test #'equal) (al t))
	     (t (l (cdr m)) (test (car m)) (al nil)))
	    (check-type test function)
	    (let ((entry (assoc x l :test test)))
	      (if entry
		  (progn (setf (cdr entry) y) m)
		(if al 
		    (cons (cons x y) m)
		  (progn
		    (setf (cdr m) (cons (cons x y) (cdr m)))
		    m))))))
  (:method ((m vector) x y)
	   (setf (elt m x) y))
  (:method ((m hash-table) x y)
	   (setf (gethash x m) y)))

(defgeneric make-value-undefined (m x)
  (:documentation "make-value-undefined M X.
Make M not be defined at X, and return the resulting mapping.  Should not be called by external code - use make-undefined instead.")
  (:method ((m list) x)
	   (if (is-assoc-list m)
	       (delete x m :key #'car :test #'equal)
	     (let ((test (car m))
		   (l (cdr m)))
	       (check-type test function)
	       (cons test (delete x l :key #'car :test test)))))
  (:method ((m vector) x)
	     (progn
	       (setf (elt m x) 'mapping-value-undefined)
	       m))
  (:method ((m hash-table) x)
	   (remhash x m)
	   m))


(defmacro make-undefined (m x &optional (defined nil def-supp))
  "Macro make-undefined M X &optional (CHECK-DEFINED nil).  Make M be undefined at X.  If CHECK-DEFINED, first check that the macro is defined at X."
  `(progn
     ,@(when def-supp
	 `((assert (or (not ,defined) (defined? ,m ,x)) ()
	     "Mapping ~a is, unexpectedly, not defined at ~a"
	     ,m ,defined ,x)))
     (_f make-value-undefined ,m ,x)))
     
  
	   
	   
	   
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; domain
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defgeneric domain (m)
  (:documentation "domain MAPPING.  Return set of keys at which MAPPING is defined.  Not implemented for MAPPINGs that are functions.")
  (:method ((m list))
	   (mapcar #'car (if (is-assoc-list m) m (progn (check-type (car m) function) (cdr m)))))
  (:method ((m vector))
	   (length m))
  (:method ((m hash-table))
	   (hash-keys m)))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; iteration
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmacro do-entries ((key-var val-var m &optional (result-form nil) (count-var (gensym))) &body body)
  "Macro do-entries (KEY-VAR VAL-PLACE MAPPING &optional (RESULT-FORM nil) (COUNT-VAR nil)) &rest BODY
Iteration macro for mappings

KEY-VAR - a symbol (not evaluated)
VAL-PLACE - a symbol (not evaluated)
MAPPING - a mapping
RESULT-FORM - a form
COUNT-VAR - a symbol (not evaluated)
BODY - a list of forms surrounded by an implicit progn

Repeatedly execute the forms in BODY.  On each iteration, KEY-VAR is bound to one of the keys of MAPPING and the symbol VAL-PLACE is a place that refers to the corresponding value (i.e., the symbol is lexically bound to the value, and also can be setf-ed to destructively change the value of that entry in the mapping).  On the first iteration, COUNT-VAR is bound to 0, on the second iteration it is 1, and so on.  Within the RESULT-FORM, COUNT-VAR is bound to the number of entries seen.  The RESULT-FORM is evaluated after the last iteration, and its value(s) returned.  The consequences are undefined if the BODY modifies MAPPING destructively in any way apart from changing the value of VAL-PLACE.

The key var is declared ignorable in the body, so you don't need a declare to not use it.

This operation is not defined for mappings that are of type function."

  (with-gensyms (entry mapping  v)
    `(let ((,mapping ,m))
       (etypecase ,mapping
	 ((satisfies is-assoc-list)
	  (let ((,count-var 0))
	    (dolist (,entry ,mapping ,result-form)
	      (let ((,key-var (car ,entry)))
		(declare (ignorable ,key-var))
		(symbol-macrolet ((,val-var (cdr ,entry)))
		  ,val-var
		  ,@body))
	      (incf ,count-var))))
	 (vector
	  (dotimes (,count-var (length ,mapping) ,result-form)
	    (let ((,key-var ,count-var))
	      (declare (ignorable ,key-var))
	      (symbol-macrolet ((,val-var (aref ,mapping ,count-var)))
		,val-var
		,@body))))
	 (list
	  (let ((,count-var 0))
	    (check-type (car ,mapping) function)
	    (dolist (,entry (cdr ,mapping) ,result-form)
	      (let ((,key-var (car ,entry)))
		(declare (ignorable ,key-var))
		(symbol-macrolet ((,val-var (cdr ,entry)))
		  ,val-var
		  ,@body))
	      (incf ,count-var))))
	 (hash-table
	  (let ((,count-var 0))
	    (maphash 
	     #'(lambda (,key-var ,v)
		 (declare (ignore ,v))
		 (declare (ignorable ,key-var))
		 (symbol-macrolet ((,val-var (gethash ,key-var ,mapping)))
		   ,val-var
		   ,@body)
		 (incf ,count-var))
	     ,mapping)
	    ,result-form))
	 (function (assert "do-entries not supported for functions."))))))
	   


(defgeneric transform (m f &key result-type)
  (:documentation "transform MAPPING FUNCTION &key (RESULT-TYPE ':same)

Return a new mapping consisting of the keys of MAPPING, but with each value replaced by FUNCTION applied to K and V.")
  (:method ((m list) f &key (result-type ':same))
	   (assert (and (eq result-type ':same) (is-assoc-list m)))
	   (transform-to-list m f)))

(defun transform-to-list (m f)
  (let ((l nil))
    (do-entries (k v m l)
      (push (cons k (funcall f k v)) l))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; internal
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun is-assoc-list (s)
  "is-assoc-list SEQUENCE.  Return t iff SEQUENCE is a list, and first element is a pair."
  (or (null s)
      (and (listp s)
	   (consp (first s)))))