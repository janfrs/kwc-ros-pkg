(in-package set)

(defclass <directory-set> (<numbered-set>)
  ((directory :initarg :dir :reader dir)
   (num-files :initarg :num-files :reader num-files))
  (:documentation "<directory-set>.  A kind of <numbered-set> that is represented using the file system.  

Initargs
:dir
:num-files

Upon creation, the directory must exist, and must contain files 0, 1, ..., NUM-FILES-1.  The Ith element in the set is the first object stored in file I.  The advantage of a <directory-set> is that, for most operations, only one object of the set needs to be present in memory at any point."))


(defmethod member? (item (s <directory-set>) &aux (test (equality-test s)))
  (dotimes (i (size s) nil)
    (when (funcall test item (item i s))
      (return t))))

(defmethod item (n (s <directory-set>))
  (read-object-from-file (format nil "~a/~a" (dir s) n)))

(defmethod item-number (item s &aux (test (equality-test s)))
  (dotimes (i (size s))
    (when (funcall test item (item i s))
      (return-from item-number i)))
  (error 'item-not-in-set :item item :set s))

(defmethod size ((s <directory-set>) &optional (constant-time nil))
  (declare (ignore constant-time))
  (num-files s))
