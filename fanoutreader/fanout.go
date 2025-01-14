package fanoutreader

import (
	"io"
	"sync"
)

// Fanout allows fanout arbitrary number of reader streams concurrently
// from one data source with known total size,
// using channel and memory buffer.
type Fanout struct {
	source  io.ReadCloser
	size    int
	current int
	buf     []byte
	err     error
	lock    sync.RWMutex
	once    sync.Once
	readers []*reader
}

// reader io.ReadCloser spawned via Fanout
type reader struct {
	fanout        *Fanout
	channel       chan []byte
	buf           []byte
	current       int
	channelClosed bool
	readerClosed  bool
}

// New Fanout factory via single io.ReadCloser source with known size
func New(source io.ReadCloser, size int) *Fanout {
	return &Fanout{
		source: source,
		size:   size,
		buf:    make([]byte, size),
	}
}

// do triggers reading data from source
func (f *Fanout) do() {
	f.once.Do(func() {
		go f.readAll()
	})
}

func (f *Fanout) readAll() {
	defer func() {
		_ = f.source.Close()
	}()
	for f.current < f.size {
		b := f.buf[f.current:]
		n, e := f.source.Read(b)
		if f.current+n > f.size {
			n = f.size - f.current
		}
		var bn []byte
		if n > 0 {
			bn = b[:n]
		}
		f.lock.Lock()
		f.current += n
		if e != nil {
			if e == io.EOF {
				e = nil
			} else {
				f.err = e
			}
			if n == 0 {
				if f.current < f.size {
					f.buf = f.buf[:f.current]
				}
				f.size = f.current
			}
		}
		readersCopy := f.readers
		f.lock.Unlock()
		f.lock.RLock()
		for _, r := range readersCopy {
			if !r.channelClosed {
				r.channel <- bn
			}
		}
		f.lock.RUnlock()
	}
}

// NewReader spawns new io.ReadCloser
func (f *Fanout) NewReader() io.ReadCloser {
	r := &reader{}
	r.channel = make(chan []byte, f.size/4096+1)
	r.fanout = f

	f.lock.Lock()
	r.buf = f.buf[:f.current]
	f.readers = append(f.readers, r)
	f.lock.Unlock()
	return r
}

// Read implements the io.Reader interface.
func (r *reader) Read(p []byte) (n int, err error) {
	r.fanout.do()
	if r.readerClosed {
		return 0, io.ErrClosedPipe
	}
	r.fanout.lock.RLock()
	e := r.fanout.err
	size := r.fanout.size
	closed := r.channelClosed
	r.fanout.lock.RUnlock()
	for {
		if r.current >= size {
			if e != nil {
				return 0, e
			}
			return 0, io.EOF
		}
		if closed {
			return 0, io.ErrClosedPipe
		}
		if len(r.buf) == 0 {
			r.buf = <-r.channel
		}
		nn := copy(p[n:], r.buf)
		if nn == 0 {
			return
		}
		r.buf = r.buf[nn:]
		r.current += nn
		n += nn
		if r.current >= size {
			_ = r.close(false)
			return
		}
	}
}

// close reader or just closing the underlying channel
func (r *reader) close(closeReader bool) (e error) {
	r.fanout.lock.Lock()
	e = r.fanout.err
	r.readerClosed = closeReader
	if r.channelClosed {
		r.fanout.lock.Unlock()
	} else {
		r.channelClosed = true
		r.fanout.lock.Unlock()
		close(r.channel)
	}
	return
}

// Close implements the io.Closer interface.
func (r *reader) Close() error {
	return r.close(true)
}
