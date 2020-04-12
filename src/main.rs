#![no_std]
#![feature(start)]
#![feature(alloc_error_handler)]

extern crate alloc;

use core::alloc::{GlobalAlloc, Layout};

use core::cell::UnsafeCell;
use core::ptr::NonNull;
use game::Game;

mod game;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

#[alloc_error_handler]
fn alloc_error_handler(_: Layout) -> ! {
    loop {}
}

#[global_allocator]
static ALLOCATOR: Heap = Heap::empty();

static REG_IE: usize = 0x0400_0200;
static REG_VCOUNT: usize = 0x0400_0006;

extern "C" {
    static mut __bss_start: u8;
    static mut __bss_end: u8;
    static mut __data_start: u8;
    static mut __data_end: u8;
    static __sidata: u8;
    static __wram_end: u8;
}

fn init_heap() {
    unsafe {
        let heap_start = &__bss_end as *const u8 as usize;
        let heap_end = &__wram_end as *const u8 as usize;
        let heap_size = heap_end - heap_start;

        ALLOCATOR.init(heap_start, heap_size);
    }
}

fn wait_for_vsync() {
    unsafe {
        while core::ptr::read_volatile(REG_VCOUNT as *const u32) >= 160 {}
        while core::ptr::read_volatile(REG_VCOUNT as *const u32) < 160 {}
    }
}

#[start]
fn main(_argc: isize, _argv: *const *const u8) -> isize {
    unsafe {
        let count = &__bss_end as *const u8 as usize - &__bss_start as *const u8 as usize;
        core::ptr::write_bytes(&mut __bss_start as *mut u8, 0, count);
        let count = &__data_end as *const u8 as usize - &__data_start as *const u8 as usize;
        core::ptr::copy_nonoverlapping(&__sidata as *const u8, &mut __data_start as *mut u8, count);
    }

    init_heap();
    let mut game: Game = Game::new(40, 60);
    unsafe {
        (0x400_0000 as *mut u16).write_volatile(0x0403); // BG2 Mode 3
        loop {
            let field = game.next();
            wait_for_vsync();
            for (i, cell) in field.iter().enumerate() {
                let col = i % 60;
                let row = i / 60;
                let color = if *cell { 0x7FFF } else { 0x0000 };
                for j in 0..4 {
                    (0x600_0000 as *mut u16)
                        .offset(((row * 4 + j) * 240 + col * 4) as isize)
                        .write_volatile(color);
                    (0x600_0000 as *mut u16)
                        .offset(((row * 4 + j) * 240 + col * 4 + 1) as isize)
                        .write_volatile(color);
                    (0x600_0000 as *mut u16)
                        .offset(((row * 4 + j) * 240 + col * 4 + 2) as isize)
                        .write_volatile(color);
                    (0x600_0000 as *mut u16)
                        .offset(((row * 4 + j) * 240 + col * 4 + 3) as isize)
                        .write_volatile(color);
                }
            }
        }
    }
}

pub struct Heap {
    heap: Mutex<linked_list_allocator::Heap>,
}

impl Heap {
    pub const fn empty() -> Heap {
        Heap {
            heap: Mutex::new(linked_list_allocator::Heap::empty()),
        }
    }

    pub unsafe fn init(&self, start_addr: usize, size: usize) {
        self.heap.lock(|heap| heap.init(start_addr, size));
    }
}

unsafe impl GlobalAlloc for Heap {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        self.heap
            .lock(|heap| heap.allocate_first_fit(layout))
            .ok()
            .map_or(0 as *mut u8, |allocation| allocation.as_ptr())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        self.heap.lock(|heap| heap.deallocate(NonNull::new_unchecked(ptr), layout));
    }
}

pub struct Mutex<T> {
    inner: UnsafeCell<T>,
}

impl<T> Mutex<T> {
    pub const fn new(value: T) -> Self {
        Mutex {
            inner: UnsafeCell::new(value),
        }
    }
}

impl<T> Mutex<T> {
    pub fn lock<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&mut T) -> R,
    {

        unsafe {
            let ie = core::ptr::read_volatile(REG_IE as *const u16);
            let ret = f(&mut *self.inner.get());
            (REG_IE as *mut u16).write_volatile(ie);
            ret
        }
    }
}

unsafe impl<T> Sync for Mutex<T> {}
