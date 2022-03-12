Assignment 1 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

## 1. Program Structure and Design of the StreamReassembler

### 1.1. Structure of the StreamReassembler

I add both attribute and method in StreamReassembler like below.

```c++
ByteStream _output;
size_t _capacity;
size_t _unassm_base;
size_t _unassm_size;

bool _eof;

std::deque<char> _buffer;
std::deque<bool> _buffer_check;

size_t _get_upperbound() const;
```
+ **_unassm_base**: tracking the first position of not assembled data.
+ **_unassm_size**: tracking the total size of not assembled data saved in the _buffer.
+ **_eof**: this flag set true when end parts of string is totally saved in the _buffer. However, it doesn't mean that whole string is received when _eof set true.
+ **_buffer**: intermediate buffer between TCP structure and bytestream. Whenever it is ready to send data into bytestream, it will do with tracking both _unassm_base and _unassem_size.
+ **_buffer_check**: validation bit table for _buffer.

### 1.2. Algorithm of the StreamReassembler

push_substring routine divides three parts

+ push data into buffer
    ```c++
    const size_t data_begin = max(index, _unassm_base);
    const size_t data_expect = max(index + data.length(), _unassm_base);
    const size_t data_end = min(data_expect, _get_upperbound());

    // push data into buffer
    _eof |= (data_expect == data_end) && eof;
    for (size_t i = data_begin; i < data_end; i++) {
        const size_t data_idx = i - index;
        const size_t buffer_idx = i - _unassm_base;

        if (!_buffer_check[buffer_idx]) {
            _buffer[buffer_idx] = data[data_idx];
            _buffer_check[buffer_idx] = true;
            _unassm_size++;
        }
    }
    ```
    Before pushing data, it is important to check how many bytes _buffer can save. In this progress, I defined new terminology like below
    + **data_begin**: index of string that can be written on the first position of _buffer
    + **data_expect**: last index of string that can be written on the ideal buffer and it maybe excess the capacity of _buffer
    + **data_end**: last index of string that can be written on the _buffer. If input string excess the capacity of _buffer, part of string is saved in _buffer.

    By using above terminolgy, _buffer save data with tracking _unassm_size and _eof set by its definition.

+ move data from buffer to bytestream
    ```c++
    string concat_data;
    for (size_t i = 0; i < _capacity && _buffer_check.front(); i++) {
        concat_data += _buffer.front();
        _unassm_base++;
        _unassm_size--;

        _buffer.pop_front();
        _buffer.push_back(0);

        _buffer_check.pop_front();
        _buffer_check.push_back(false);
    }
    _output.write(concat_data);
    ```
    After pushing data, intermediate buffer(_buffer) start to send data into bytestream when ready to assemble data with tracking _unassm_base and _unassem_size.

    It is important step to restore total size of _buffer, so whenever pop, push with initial value is essential.

+ check input is terminated
    ```c++
    if (_unassm_size == 0 && _eof)
        _output.end_input();
    ```
    After sending data, check whether input is terminated. It is straightforward by using  _unassem_size and _eof, because _unassem_size means there is no pending data and _eof means end part of string is received.

## 2. Implementation Challenges:

In my case, there are two challenges.

### 2.1. Processing with three different indices

In push_string function, it has to processing with three different indices like below
+ Absolute index of whole string
+ Index of received string
+ Index of _buffer

Among three indices, I had to be careful not to make bugs.

### 2.2. Meaning of end of file (_eof)

It is important what _eof means. Different definition of _eof makes different algorithm. I contemplated how many meaning I put in _eof. After contemplation, my approach is just naive one that _eof set true when end part of string is successfully saved.

## 3. Remaining Bugs

Actually, I didn't find any bugs that reamin in the code.

## Optional

- Optional: I had unexpected difficulty with: There is no difficulty.

- Optional: I think you could make this assignment better by: It is already enough to understand.

- Optional: I was surprised by: This assignment has systematic design (well design assignment!)

- Optional: I'm not sure about: Nothing about it.
