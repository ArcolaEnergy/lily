#include "lily.hpp"
#include "lilyConstruct.hpp"
#include "lilyDefaultEnvironment.hpp"


template <typename LilyT, typename T>
static
T _lilyFold(LilyList* vs, std::function<T(LilyT*,T)> fn, T start) {
	T res= start;
	while (true) {
		if (auto pair= dynamic_cast<LilyPair*>(vs)) {
			if (auto v=  UNWRAP_AS(LilyT, pair->_car)) {
				res= fn(v, res);
				vs= LIST_UNWRAP(pair->_cdr);
			} else {
				throw std::logic_error("not an integer");
			}
		} else if (dynamic_cast<LilyNull*>(vs)) {
			return res;
		} else {
			throw std::logic_error(STR("not a proper list, ending in: "
						   << show(vs)));
		}
	}
}

#define DEF_FOLD_UP_NATIVE(name, LilyT, T, OP, START)			\
	static								\
	LilyObjectPtr name(LilyObjectPtr vs,				\
			   LilyObjectPtr _ctx,				\
			   LilyObjectPtr _cont) {			\
		return LILY_NEW						\
			(LilyT(_lilyFold<LilyT, T>			\
			       (XLIST_UNWRAP(vs),			\
				[](LilyT* num, T res) -> T {		\
				       /* XX check for overflow! */	\
				       return res OP num->value;	\
			        },					\
				START)));				\
	}

DEF_FOLD_UP_NATIVE(lilyAdd, LilyInt64, int64_t, +, 0);
DEF_FOLD_UP_NATIVE(lilyMult, LilyInt64, int64_t, *, 1);

#define DEF_FOLD_DOWN_NATIVE(name, LilyT, T, OP)				\
	static								\
	LilyObjectPtr name(LilyObjectPtr vs,				\
			   LilyObjectPtr _ctx,				\
			   LilyObjectPtr _cont) {			\
		LilyList* _vs= XLIST_UNWRAP(vs);			\
		return LILY_NEW						\
			(LilyT(_lilyFold<LilyT, T>			\
			       (XLIST_UNWRAP(_vs->rest()),		\
				[](LilyT* num, T res) -> T {		\
				       /* XX check for overflow? */	\
				       return res OP num->value;	\
			        },					\
				XUNWRAP_AS(LilyT, _vs->first())->value))); \
	}

DEF_FOLD_DOWN_NATIVE(lilySub, LilyInt64, int64_t, -);
DEF_FOLD_DOWN_NATIVE(lilyQuotient, LilyInt64, int64_t, /);
DEF_FOLD_DOWN_NATIVE(lilyRemainder, LilyInt64, int64_t, %);

// DEF_NATIVE(lilyDiv, LilyInt64, int64_t, /); // generic


static
LilyObjectPtr lilyCons(LilyObjectPtr vs,
		       LilyObjectPtr _ctx,
		       LilyObjectPtr _cont) {
	// WARN("cons: "<<show(vs));
	LETU_AS(vs0, LilyPair, vs);
	if (vs0) {
		LETU_AS(vs1, LilyPair, vs0->_cdr);
		if (vs1) {
			LETU_AS(vs2, LilyNull, vs1->_cdr);
			if (vs2) {
				return CONS(vs0->_car, vs1->_car);
			}
		}
	}
	throw std::logic_error("cons needs 2 arguments");
}

static LilyObjectPtr lilyCar(LilyObjectPtr vs,
			     LilyObjectPtr _ctx,
			     LilyObjectPtr _cont) {
	return apply1ary("car", [](LilyObjectPtr v) {
			LETU_AS(p, LilyPair, v);
			if (p)
				return p->_car;
			else
				throw std::logic_error(STR("not a pair: "
							   << show(v)));
		}, vs);
}

static LilyObjectPtr lilyCdr(LilyObjectPtr vs,
			     LilyObjectPtr _ctx,
			     LilyObjectPtr _cont) {
	return apply1ary("cdr", [](LilyObjectPtr v) {
			LETU_AS(p, LilyPair, v);
			if (p)
				return p->_cdr;
			else
				throw std::logic_error(STR("not a pair: "
							   << show(v)));
		}, vs);
}

static LilyObjectPtr lilyQuote(LilyObjectPtr es,
			       LilyObjectPtr _ctx,
			       LilyObjectPtr _cont) {
	return apply1ary("quote", [](LilyObjectPtr e) {
			WARN("quote: " << show(e));
			return e;
		}, es);
}

static LilyObjectPtr lilyLength(LilyObjectPtr arguments,
				LilyObjectPtr _ctx,
				LilyObjectPtr _cont) {
	// arguments == ((12 13 14))
	LET_AS(_arguments, LilyList, arguments);
	if (is_LilyNull(&*(_arguments->rest()))) {
		// we have just one argument, cool.
		auto l= _arguments->first(); // (12 13 14) or "hello" or something
		int64_t len= 0;
		while (true) {
			LET_AS(p, LilyPair, l);
			if (p) {
				len++; // check overflow? 64bit int is pretty large though :)
				l= p->cdr();
			} else {
				LET_AS(null, LilyNull, l);
				if (null) {
					break;
				} else {
					throw std::logic_error
						(STR("not a list: "
						     << show(_arguments->first())));
				}
			}
		}
		return INT(len);
	} else {
		throw std::logic_error("length received more than one argument");
	}
		
}

static LilyObjectPtr lilyList(LilyObjectPtr arguments,
			      LilyObjectPtr _ctx,
			      LilyObjectPtr _cont) {
	return arguments;
}

static LilyObjectPtr lilyReverse(LilyObjectPtr arguments,
				 LilyObjectPtr _ctx,
				 LilyObjectPtr _cont) {
	LET_AS(_arguments, LilyList, arguments);
	return reverse(_arguments->first()); // XX better arg count checking
}


LilyListPtr lilyDefaultEnvironment() {
	static LilyListPtr env= LIST(
		PAIR(SYMBOL("+"), NATIVE_PROCEDURE(lilyAdd, "+")),
		PAIR(SYMBOL("*"), NATIVE_PROCEDURE(lilyMult, "*")),
		PAIR(SYMBOL("-"), NATIVE_PROCEDURE(lilySub, "-")),
		PAIR(SYMBOL("quotient"), NATIVE_PROCEDURE(lilyQuotient, "quotient")),
		PAIR(SYMBOL("remainder"), NATIVE_PROCEDURE(lilyRemainder, "remainder")),
		// PAIR(SYMBOL("integer./"), NATIVE_PROCEDURE(lilyDiv, "integer./")),
		PAIR(SYMBOL("cons"), NATIVE_PROCEDURE(lilyCons, "cons")),
		PAIR(SYMBOL("car"), NATIVE_PROCEDURE(lilyCar, "car")),
		PAIR(SYMBOL("cdr"), NATIVE_PROCEDURE(lilyCdr, "cdr")),
		PAIR(SYMBOL("quote"), NATIVE_EVALUATOR(lilyQuote, "quote")),
		PAIR(SYMBOL("list"), NATIVE_PROCEDURE(lilyList, "list")),
		PAIR(SYMBOL("length"), NATIVE_PROCEDURE(lilyLength, "length")),
		PAIR(SYMBOL("reverse"), NATIVE_PROCEDURE(lilyReverse, "reverse")),
		);
	return env;
}


