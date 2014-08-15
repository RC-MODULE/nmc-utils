//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//              Базовое программное обеспечение нейропроцессора             //
//                                                                          //
//              Библиотека времени выполнения                               //
//                                                                          //
//                          стартовый код                                   //
//                                                                          //
//    $Revision:: 4     $      $Date:: 23.01.12 16:23   $					          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// АВТОР: Челюбеев А.А.


global start  : label;	// Стандартная точка входа.
global _exit  : label;	// Функция exit.
global __exit : label;	// Функция _exit.


weak _sync : label; // функция sync для варианта программного эмулятора

extern stop  : label; // Внешняя функция остановки программы.

extern __main :label;
extern ctor : label;
extern dtor : label;
extern _easynmc_argc: word;
extern _easynmc_argv: word;


nobits stack
    STACK :long;     // символ начала стека
end stack;

begin text

         // Identification string
    CompilerString : long[4] =
            'NeuroMatrix(r) NM640x SDK v3.00';

        // место в памяти для хранения кода возврата программы
    result_code : word = -1;
    dummy : word = -1;

// Точка входа программы
<start>
    sp = STACK;								//  start +0
    delayed goto begin_prog with gr7=false;	//  start +2
    push gr7;								//  start +4,	argv =0
    push gr7;								//  start +5,	argc =0
				//  start +6 -признак, по которому останавливаются эмуляторы						
<int_stop>
    goto stop;  // полный останов. В gr7 и gr0 - код возврата
<_sync>         // функция sync в варианте программного эмулятора
    return;

    // последовательность выше не должна нарушаться
    // start        - начало программы
    // start + 6    - точка останова програмы
    // start + 10   - точка функции sync для программного эмулятора

<begin_prog>
    call ctor;
    gr7 = [_easynmc_argv];     /* argv */
    push gr7;      
    gr7 = [_easynmc_argc];     /* argc */
    push gr7;     
    call __main;
    push ar7,gr7;

<terminate>
    call dtor;
    pop ar5,gr5;
    gr0 = gr5 with gr7 = gr5;
    [ result_code ] = gr7;
    goto int_stop;

<_exit>
    sp -= 2;
    goto terminate;

<__exit>
    sp -= 2;
    pop ar5,gr5;
    gr0 = gr5 with gr7 = gr5;
    [ result_code ] = gr7;
    goto int_stop;

end text;
