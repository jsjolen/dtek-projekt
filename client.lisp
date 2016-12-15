;;   Copyright (c)  2016 Johan Sjolen

;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:

;; 1. Redistributions of source code must retain the above copyright
;;    notice, this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;; 3. The name of the author may not be used to endorse or promote products
;;    derived from this software without specific prior written permission.

;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
;; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
;; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
;; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
;; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
;; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


(require 'asdf)
(defvar out)
;; Whole file: Johan Sjolen
(defun start-communication ()
  (setf out (open "/dev/ttyUSB0" :element-type 'base-char :direction :io :if-exists :append))
  (format t "OK, starting communication...~%")
  (sleep 5)
  ;(uiop:run-program "stty -F /dev/ttyUSB0 115200")
  )
(defun read-string-no-hang (&optional (stream out))
  (loop for x = (read-char-no-hang stream nil nil)
     until (eq x nil)
     collect x))
(defun read-string (&optional (stream out))
	   (loop for x = (read-char stream nil nil)
	      until (eq x nil)
	      collect x))
(defun read-command (&optional (stream out))
  (coerce  (list (read-char stream)
		 (read-char stream)
		 (read-char stream))
	   'string))
(defun send-string (str &optional (stream out))
	   (loop for c across str
		do
		(write-char c stream))
	   (force-output stream))
(defun clear-screen ()
  (send-string "CLE"))

(defun consult-python (program)
  (with-input-from-string (i program)
    (with-output-to-string (o)
      (uiop:run-program "python3 lex.py" :input i :output o))))

(defun send-program (program)
  (let ((code (consult-python program)))
    (send-string "STA")
    (format t "Waiting for ACK...~%")
    (assert (string=  "ACK" (read-command)))
    (format t "got ACK, sending code...~%")
    (send-string code)
    (send-string "END")
    (format t "Waiting for ACK...~%")
    (assert (string=  "ACK" (read-command)))
    (format t "got ACK, done!~%")))

(defun receive-bitmap ()
  (send-string "SEN")
  (let* ((dim (* 128 32))
	 (bitmap (make-array dim)))
    (loop for x from 0 to (1- dim)
	 do
	 (setf (elt bitmap x)
	       (if (char= #\Nul (read-char out))
		   0
		   1)))
    bitmap))


(defun do-program (p  &optional (format-args nil) (close-p nil))
  (unwind-protect
       (progn (when (not (open-stream-p out)) (start-communication))
	      (send-program (apply #'format nil p format-args))))
  (when close-p (close out)))


(defun test ()
  (do-program "DOWN. LEFT 90. FORW 10. LEFT 90. FORW 20. LEFT 90. FORW 10. LEFT 90. FORW 10."))
(defun test2 ()
  (do-program "DOWN. LEFT 90. FORW 10. RIGHT 90. FORW 100."))
(defun main (&optional args)
  (declare (ignorable args))
  (when (not (open-stream-p out)) (start-communication))
  (with-output-to-string (s)
    (loop for line = (read-line *standard-input* nil 'eof) until (eq line 'eof) do
	 (format s line))))

